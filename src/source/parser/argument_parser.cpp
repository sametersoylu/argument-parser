#include "argument_parser.hpp"

#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class deferred_exec {
public:
	deferred_exec(std::function<void()> const &func) : func(func) {}
	~deferred_exec() {
		func();
	}

private:
	std::function<void()> func;
};

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
			auto short_arg =
				reverse_short_arguments.find(id) != reverse_short_arguments.end() ? reverse_short_arguments.at(id) : "";
			auto long_arg =
				reverse_long_arguments.find(id) != reverse_long_arguments.end() ? reverse_long_arguments.at(id) : "";

			ss << "\t";
			std::unordered_set<std::string> hasOnce;
			for (auto const &convention : convention_types) {
				auto generatedHelpText = convention->make_help_text(short_arg, long_arg, arg.expects_parameter());
				if (hasOnce.find(generatedHelpText) == hasOnce.end()) {
					ss << generatedHelpText << "\t";
					hasOnce.insert(generatedHelpText);
				}
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

	void base_parser::enforce_creation_thread() {
		if (std::this_thread::get_id() != this->creation_thread_id.load()) {
			throw std::runtime_error("handle_arguments must be called from the main thread");
		}
	}

	bool base_parser::test_conventions(std::initializer_list<conventions::convention const *const> convention_types,
									   std::unordered_map<std::string, std::string> &values_for_arguments,
									   std::vector<std::pair<std::string, argument>> &found_arguments,
									   std::optional<argument> &found_help, std::vector<std::string>::iterator it,
									   std::stringstream &error_stream) {

		std::string current_argument = *it;

		for (auto const &convention_type : convention_types) {
			auto extracted = convention_type->get_argument(current_argument);
			if (extracted.first == conventions::argument_type::ERROR) {
				error_stream << "Convention \"" << convention_type->name() << "\" failed with: " << extracted.second
							 << "\n";
				continue;
			}

			try {
				argument &corresponding_argument = get_argument(extracted);

				if (extracted.second == "h" || extracted.second == "help") {
					found_help = corresponding_argument;
					return true;
				}

				found_arguments.emplace_back(extracted.second, corresponding_argument);

				if (corresponding_argument.expects_parameter()) {
					if (convention_type->requires_next_token() && (it + 1) == parsed_arguments.end()) {
						throw std::runtime_error("Expected value for argument " + extracted.second);
					}
					values_for_arguments[extracted.second] =
						convention_type->requires_next_token() ? *(++it) : convention_type->extract_value(*it);
				}

				return true;
			} catch (const std::runtime_error &e) {
				error_stream << "Convention \"" << convention_type->name() << "\" failed with: " << e.what() << "\n";
			}
		}

		return false;
	}

	void base_parser::extract_arguments(std::initializer_list<conventions::convention const *const> convention_types,
										std::unordered_map<std::string, std::string> &values_for_arguments,
										std::vector<std::pair<std::string, argument>> &found_arguments,
										std::optional<argument> &found_help) {

		for (auto it = parsed_arguments.begin(); it != parsed_arguments.end(); ++it) {
			std::stringstream error_stream;

			if (!test_conventions(convention_types, values_for_arguments, found_arguments, found_help, it,
								  error_stream)) {
				throw std::runtime_error("All trials for argument: \n\t\"" + *it + "\"\n failed with: \n" +
										 error_stream.str());
			}
		}
	}

	void base_parser::invoke_arguments(std::unordered_map<std::string, std::string> const &values_for_arguments,
									   std::vector<std::pair<std::string, argument>> &found_arguments,
									   std::optional<argument> const &found_help) {

		if (found_help) {
			found_help->action->invoke();
			return;
		}

		std::stringstream error_stream;
		for (auto &[key, value] : found_arguments) {
			try {
				if (value.expects_parameter()) {
					value.action->invoke_with_parameter(values_for_arguments.at(key));
				} else {
					value.action->invoke();
				}
				value.set_invoked(true);
			} catch (const std::runtime_error &e) {
				error_stream << "Argument " << key << " failed with: " << e.what() << "\n";
			}
		}

		std::string error_message = error_stream.str();
		if (!error_message.empty()) {
			throw std::runtime_error(error_message);
		}
	}

	void base_parser::handle_arguments(std::initializer_list<conventions::convention const *const> convention_types) {
		enforce_creation_thread();

		deferred_exec reset_current_conventions([this]() { this->reset_current_conventions(); });
		this->current_conventions(convention_types);

		std::unordered_map<std::string, std::string> values_for_arguments;
		std::vector<std::pair<std::string, argument>> found_arguments;
		std::optional<argument> found_help = std::nullopt;

		extract_arguments(convention_types, values_for_arguments, found_arguments, found_help);
		invoke_arguments(values_for_arguments, found_arguments, found_help);
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
		if (short_arg != "-") {
			short_arguments[short_arg] = id;
			reverse_short_arguments[id] = short_arg;
		}
		if (long_arg != "-") {
			long_arguments[long_arg] = id;
			reverse_long_arguments[id] = long_arg;
		}
	}

	std::string get_one_name(std::string const &short_name, std::string const &long_name) {
		std::string res{};
		if (short_name != "-") {
			res += short_name;
		}

		if (long_name != "-") {
			if (!res.empty()) {
				res += ", ";
			}

			res += long_name;
		}
		return res;
	}

	void base_parser::check_for_required_arguments(
		std::initializer_list<conventions::convention const *const> convention_types) {
		std::vector<std::tuple<std::string, std::string, bool>> required_args;
		for (auto const &[key, arg] : argument_map) {
			if (arg.is_required() && !arg.is_invoked()) {
				auto short_arg = reverse_short_arguments.find(key) != reverse_short_arguments.end()
									 ? reverse_short_arguments.at(key)
									 : "-";
				auto long_arg = reverse_long_arguments.find(key) != reverse_long_arguments.end()
									? reverse_long_arguments.at(key)
									: "-";

				required_args.emplace_back<std::tuple<std::string, std::string, bool>>(
					{short_arg, long_arg, arg.expects_parameter()});
			}
		}

		if (!required_args.empty()) {
			std::cerr << "These arguments were expected but not provided: \n";
			for (auto const &[s, l, p] : required_args) {
				std::cerr << "\t" << get_one_name(s, l) << ": must be provided as one of [";
				for (auto it = convention_types.begin(); it != convention_types.end(); ++it) {
					std::cerr << (*it)->make_help_text(s, l, p);
					if (it + 1 != convention_types.end()) {
						std::cerr << ", ";
					}
				}
				std::cerr << "]\n";
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