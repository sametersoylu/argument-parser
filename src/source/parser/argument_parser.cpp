#include "argument_parser.hpp"

#include <iostream>
#include <sstream>

bool contains(std::unordered_map<std::string, int> const &map, std::string const &key) {
	return map.find(key) != map.end();
}

namespace argument_parser {
	argument::argument()
		: id(0), name(), action(std::make_unique<non_parametered_action>([]() {})), required(false), invoked(false) {}

	argument::argument(const argument &other)
		: id(other.id), name(other.name), action(other.action->clone()), required(other.required),
		  invoked(other.invoked), help_text(other.help_text) {}

	argument &argument::operator=(const argument &other) {
		if (this != &other) {
			id = other.id;
			name = other.name;
			action = other.action->clone();
			required = other.required;
			invoked = other.invoked;
			help_text = other.help_text;
		}
		return *this;
	}

	bool argument::expects_parameter() const {
		return action->expects_parameter();
	}

	bool argument::is_required() const {
		return required;
	}

	bool argument::is_invoked() const {
		return invoked;
	}

	std::string argument::get_name() const {
		return name;
	}

	std::string argument::get_help_text() const {
		return help_text;
	}

	void argument::set_required(bool val) {
		required = val;
	}

	void argument::set_invoked(bool val) {
		invoked = val;
	}

	void argument::set_help_text(std::string const &text) {
		help_text = text;
	}

	void base_parser::on_complete(std::function<void(base_parser const &)> const &handler) {
		on_complete_events.emplace_back(handler);
	}

	std::string
	base_parser::build_help_text(std::initializer_list<conventions::convention const *const> convention_types) const {
		std::stringstream ss;
		ss << "Usage: " << program_name << " [OPTIONS]...\n";

		for (auto const &[id, arg] : argument_map) {
			auto short_arg = reverse_short_arguments.at(id);
			auto long_arg = reverse_long_arguments.at(id);
			ss << "\t";
			for (auto const &convention : convention_types) {
				ss << convention->short_prec() << short_arg << ", " << convention->long_prec() << long_arg << "\t";
			}
			ss << arg.help_text << "\n";
		}
		return ss.str();
	}

	argument &base_parser::get_argument(conventions::parsed_argument const &arg) {
		if (arg.first == conventions::argument_type::LONG) {
			auto long_pos = long_arguments.find(arg.second);
			if (long_pos != long_arguments.end())
				return argument_map.at(long_pos->second);
		} else if (arg.first == conventions::argument_type::SHORT) {
			auto short_pos = short_arguments.find(arg.second);
			if (short_pos != short_arguments.end())
				return argument_map.at(short_pos->second);
		} else if (arg.first == conventions::argument_type::INTERCHANGABLE) {
			auto long_pos = long_arguments.find(arg.second);
			if (long_pos != long_arguments.end())
				return argument_map.at(long_pos->second);
			auto short_pos = short_arguments.find(arg.second);
			if (short_pos != short_arguments.end())
				return argument_map.at(short_pos->second);
		}
		throw std::runtime_error("Unknown argument: " + arg.second);
	}

	void base_parser::handle_arguments(std::initializer_list<conventions::convention const *const> convention_types) {
		for (auto it = parsed_arguments.begin(); it != parsed_arguments.end(); ++it) {
			std::stringstream error_stream;
			bool arg_correctly_handled = false;

			for (auto const &convention_type : convention_types) {
				auto extracted = convention_type->get_argument(*it);
				if (extracted.first == conventions::argument_type::ERROR) {
					error_stream << "Convention \"" << convention_type->name() << "\" failed with: " << extracted.second
								 << "\n";
					continue;
				}

				try {
					argument &corresponding_argument = get_argument(extracted);
					if (corresponding_argument.expects_parameter()) {
						if (convention_type->requires_next_token() && (it + 1) == parsed_arguments.end()) {
							throw std::runtime_error("expected value for argument " + extracted.second);
						}
						auto value_raw =
							convention_type->requires_next_token() ? *(++it) : convention_type->extract_value(*it);
						corresponding_argument.action->invoke_with_parameter(value_raw);
					} else {
						corresponding_argument.action->invoke();
					}

					corresponding_argument.set_invoked(true);
					arg_correctly_handled = true;
					break; // Convention succeeded, move to the next argument token

				} catch (const std::runtime_error &e) {
					error_stream << "Convention \"" << convention_type->name() << "\" failed with: " << e.what()
								 << "\n";
				}
			}

			if (!arg_correctly_handled) {
				throw std::runtime_error("All trials for argument: \n\t\"" + *it + "\"\n failed with: \n" +
										 error_stream.str());
			}
		}
		check_for_required_arguments(convention_types);
		fire_on_complete_events();
	}

	void base_parser::display_help(std::initializer_list<conventions::convention const *const> convention_types) const {
		std::cout << build_help_text(convention_types);
	}

	std::optional<int> base_parser::find_argument_id(std::string const &arg) const {
		auto long_pos = long_arguments.find(arg);
		auto short_post = short_arguments.find(arg);

		if (long_pos != long_arguments.end())
			return long_pos->second;
		if (short_post != short_arguments.end())
			return short_post->second;
		return std::nullopt;
	}

	void base_parser::assert_argument_not_exist(std::string const &short_arg, std::string const &long_arg) const {
		if (contains(short_arguments, short_arg) || contains(long_arguments, long_arg)) {
			throw std::runtime_error("The key already exists!");
		}
	}

	void base_parser::set_argument_status(bool is_required, std::string const &help_text, argument &arg) {
		arg.set_required(is_required);
		arg.set_help_text(help_text);
	}

	void base_parser::place_argument(int id, argument const &arg, std::string const &short_arg,
									 std::string const &long_arg) {
		argument_map[id] = arg;
		short_arguments[short_arg] = id;
		reverse_short_arguments[id] = short_arg;
		long_arguments[long_arg] = id;
		reverse_long_arguments[id] = long_arg;
	}

	void base_parser::check_for_required_arguments(
		std::initializer_list<conventions::convention const *const> convention_types) {
		std::vector<std::pair<std::string, std::string>> required_args;
		for (auto const &[key, arg] : argument_map) {
			if (arg.is_required() && !arg.is_invoked()) {
				required_args.emplace_back<std::pair<std::string, std::string>>(
					{reverse_short_arguments[key], reverse_long_arguments[key]});
			}
		}

		if (!required_args.empty()) {
			std::cerr << "These arguments were expected but not provided: ";
			for (auto const &[s, l] : required_args) {
				std::cerr << "[-" << s << ", --" << l << "] ";
			}
			std::cerr << "\n";
			display_help(convention_types);
		}
	}

	void base_parser::fire_on_complete_events() const {
		for (auto const &event : on_complete_events) {
			event(*this);
		}
	}
} // namespace argument_parser