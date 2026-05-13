#include "argument_parser.hpp"
#include "exceptions.hpp"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class deferred_exec {
public:
	explicit deferred_exec(std::function<void()> const &func) : func(func) {}
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
	argument::argument() : id(0), action(std::make_unique<action_no_param>([] {})), required(false), invoked(false) {}

	argument::argument(const argument &other)
		: id(other.id), name(other.name), action(other.action->clone()), required(other.required),
		  invoked(other.invoked), help_text(other.help_text), positional(other.positional),
		  positional_accumulator(other.positional_accumulator), position_index(other.position_index) {}

	argument &argument::operator=(const argument &other) {
		if (this != &other) {
			id = other.id;
			name = other.name;
			action = other.action->clone();
			required = other.required;
			invoked = other.invoked;
			help_text = other.help_text;
			positional = other.positional;
			positional_accumulator = other.positional_accumulator;
			position_index = other.position_index;
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

	void argument::set_required(const bool val) {
		required = val;
	}

	void argument::set_invoked(const bool val) {
		invoked = val;
	}

	void argument::set_help_text(std::string const &text) {
		help_text = text;
	}

	bool argument::is_positional() const {
		return positional;
	}

	bool argument::is_positional_accumulator() const {
		return positional_accumulator;
	}

	std::optional<int> argument::get_position_index() const {
		return position_index;
	}

	void argument::set_positional(const bool val) {
		positional = val;
	}

	void argument::set_positional_accumulator(const bool val) {
		positional_accumulator = val;
	}

	void argument::set_position_index(const std::optional<int> idx) {
		position_index = idx;
	}

	void base_parser::on_complete(std::function<void(base_parser const &)> const &action) {
		on_complete_events.emplace_back(action);
	}

	void base_parser::on_complete(std::function<void(base_parser const &)> const &handler, const bool to_start) {
		if (to_start) {
			on_complete_events.emplace_front(handler);
		} else {
			on_complete_events.emplace_back(handler);
		}
	}

	std::string
	base_parser::build_help_text(std::initializer_list<conventions::convention const *const> convention_types) const {
		std::stringstream ss;
		ss << "Usage: " << program_name << " [OPTIONS]...";

		for (auto const &pos_id : positional_arguments) {
			if (pos_id == -1)
				continue;
			auto name_it = reverse_positional_names.find(pos_id);
			if (name_it == reverse_positional_names.end())
				continue;

			auto const &arg = argument_map.at(pos_id);
			if (arg.is_required()) {
				ss << " <" << name_it->second << ">";
			} else {
				ss << " [" << name_it->second << "]";
			}

			if (arg.is_positional_accumulator()) {
				ss << "...";
			}
		}
		ss << "\n";

		size_t max_short_len = 0;
		size_t max_long_len = 0;

		struct arg_help_info_t {
			std::vector<std::pair<std::string, std::string>> convention_parts;
			std::string desc;
		};
		std::vector<arg_help_info_t> help_lines;

		for (auto const &[id, arg] : argument_map) {
			if (arg.is_positional())
				continue;

			auto short_arg =
				reverse_short_arguments.find(id) != reverse_short_arguments.end() ? reverse_short_arguments.at(id) : "";
			auto long_arg =
				reverse_long_arguments.find(id) != reverse_long_arguments.end() ? reverse_long_arguments.at(id) : "";

			std::vector<std::pair<std::string, std::string>> parts;
			std::unordered_set<std::string> hasOnce;
			for (auto const &convention : convention_types) {
				auto generatedParts = convention->make_help_text(short_arg, long_arg, arg.expects_parameter());
				if (std::string combined = generatedParts.first + "|" + generatedParts.second;
					hasOnce.find(combined) == hasOnce.end()) {
					parts.push_back(generatedParts);
					hasOnce.insert(combined);

					if (generatedParts.first.length() > max_short_len) {
						max_short_len = generatedParts.first.length();
					}
					if (generatedParts.second.length() > max_long_len) {
						max_long_len = generatedParts.second.length();
					}
				} else {
					parts.emplace_back("", ""); // trigger empty space in the help text
				}
			}
			help_lines.push_back({parts, arg.help_text});
		}

		if (!help_lines.empty()) {
			for (const auto &[convention_parts, desc] : help_lines) {
				ss << "\t";
				for (size_t i = 0; i < convention_parts.size(); ++i) {
					const auto &[fst, snd] = convention_parts[i];
					if (i > 0) {
						ss << "  ";
					}
					ss << std::left << std::setw(static_cast<int>(max_short_len)) << fst << "  "
					   << std::setw(static_cast<int>(max_long_len)) << snd;
				}
				ss << "\t" << desc << "\n";
			}
		}

		if (!positional_arguments.empty()) {
			ss << "\nPositional arguments:\n";
			size_t max_pos_name_len = 0;
			for (auto const &pos_id : positional_arguments) {
				if (pos_id == -1)
					continue;
				if (auto name_it = reverse_positional_names.find(pos_id); name_it != reverse_positional_names.end()) {
					if (size_t display_len = name_it->second.length() + 2; display_len > max_pos_name_len)
						max_pos_name_len = display_len;
				}
			}

			for (auto const &pos_id : positional_arguments) {
				if (pos_id == -1)
					continue;
				auto name_it = reverse_positional_names.find(pos_id);
				if (name_it == reverse_positional_names.end())
					continue;
				auto const &arg = argument_map.at(pos_id);
				std::string display_name = "<" + name_it->second + ">";
				ss << "\t" << std::left << std::setw(static_cast<int>(max_pos_name_len)) << display_name << "\t"
				   << arg.get_help_text() << "\n";
			}
		}

		return ss.str();
	}

	argument &base_parser::get_argument(conventions::parsed_argument const &arg) {
		if (arg.first == conventions::argument_type::LONG) {
			if (const auto long_pos = long_arguments.find(arg.second); long_pos != long_arguments.end())
				return argument_map.at(long_pos->second);
		} else if (arg.first == conventions::argument_type::SHORT) {
			if (const auto short_pos = short_arguments.find(arg.second); short_pos != short_arguments.end())
				return argument_map.at(short_pos->second);
		} else if (arg.first == conventions::argument_type::INTERCHANGABLE) {
			if (const auto long_pos = long_arguments.find(arg.second); long_pos != long_arguments.end())
				return argument_map.at(long_pos->second);
			if (const auto short_pos = short_arguments.find(arg.second); short_pos != short_arguments.end())
				return argument_map.at(short_pos->second);
		}
		throw std::runtime_error("Unknown argument: " + arg.second);
	}

	void base_parser::enforce_creation_thread() const {
		if (std::this_thread::get_id() != this->creation_thread_id.load()) {
			throw std::runtime_error("handle_arguments must be called from the main thread");
		}
	}

	bool
	base_parser::test_conventions(const std::initializer_list<conventions::convention const *const> convention_types,
								  std::vector<found_argument> &found_arguments, std::optional<argument> &found_help,
								  std::vector<std::string>::iterator &it, std::stringstream &error_stream) {

		const std::string current_argument = *it;

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

				found_argument found{extracted.second, corresponding_argument};

				if (corresponding_argument.expects_parameter()) {
					if (convention_type->requires_next_token() && it + 1 == parsed_arguments.end()) {
						throw std::runtime_error("Expected value for argument " + extracted.second);
					}
					found.value =
						convention_type->requires_next_token() ? *(++it) : convention_type->extract_value(*it);
				}

				found_arguments.emplace_back(std::move(found));
				return true;
			} catch (const std::runtime_error &e) {
				error_stream << "Convention \"" << convention_type->name() << "\" failed with: " << e.what() << "\n";
			}
		}

		return false;
	}

	void
	base_parser::extract_arguments(const std::initializer_list<conventions::convention const *const> convention_types,
								   std::vector<found_argument> &found_arguments, std::optional<argument> &found_help) {

		size_t next_positional_index = 0;
		bool force_positional = false;

		for (auto it = parsed_arguments.begin(); it != parsed_arguments.end(); ++it) {
			if (*it == "--") {
				force_positional = true;
				continue;
			}

			if (force_positional) {
				auto slot = next_positional_slot(next_positional_index);
				if (!slot.has_value()) {
					throw std::runtime_error("Unexpected positional argument: \"" + *it + "\"");
				}
				int arg_id = positional_arguments[*slot];
				argument &pos_arg = argument_map.at(arg_id);
				std::string const &pos_name = reverse_positional_names.at(arg_id);
				found_arguments.push_back({pos_name, pos_arg, *it});
				if (!pos_arg.is_positional_accumulator()) {
					next_positional_index = *slot + 1;
				}
				continue;
			}

			if (std::stringstream error_stream;
				!test_conventions(convention_types, found_arguments, found_help, it, error_stream)) {
				if (auto slot = next_positional_slot(next_positional_index); slot.has_value()) {
					int arg_id = positional_arguments[*slot];
					argument &pos_arg = argument_map.at(arg_id);
					std::string const &pos_name = reverse_positional_names.at(arg_id);
					found_arguments.push_back({pos_name, pos_arg, *it});
					if (!pos_arg.is_positional_accumulator()) {
						next_positional_index = *slot + 1;
					}
				} else {
					if (m_settings.ignore_unknown_arguments) {
						continue;
					}
					throw parser::unknown_argument_exception("All trials for argument: \n\t\"" + *it +
															 "\"\n failed with: \n" + error_stream.str());
				}
			}
		}
	}

	std::string replace_var(std::string text, const std::string &var_name, const std::string &value) {
		const std::string placeholder = "${" + var_name + "}";
		size_t pos = text.find(placeholder);

		while (pos != std::string::npos) {
			text.replace(pos, placeholder.length(), value);
			pos = text.find(placeholder, pos + value.length());
		}
		return text;
	}

	void base_parser::invoke_arguments(std::vector<found_argument> &found_arguments,
									   std::optional<argument> const &found_help) {

		if (found_help) {
			found_help->action->invoke();
			return;
		}

		std::stringstream error_stream;
		for (auto &[key, arg, value] : found_arguments) {
			try {
				if (arg.expects_parameter()) {
					arg.action->invoke_with_parameter(value.value());
				} else {
					arg.action->invoke();
				}
				arg.set_invoked(true);
				argument_map.at(arg.id).set_invoked(true);
			} catch (const std::runtime_error &e) {
				std::string err{e.what()};
				err = replace_var(err, "KEY", "for " + key);
				error_stream << "Error: " << err << "\n";
			}
		}

		if (const std::string error_message = error_stream.str(); !error_message.empty()) {
			throw std::runtime_error(error_message);
		}
	}

	void
	base_parser::handle_arguments(const std::initializer_list<conventions::convention const *const> convention_types) {
		enforce_creation_thread();

		deferred_exec reset_current_conventions([this] { this->reset_current_conventions(); });
		this->current_conventions(convention_types);

		std::vector<found_argument> found_arguments;
		std::optional<argument> found_help = std::nullopt;

		try {
			extract_arguments(convention_types, found_arguments, found_help);
			invoke_arguments(found_arguments, found_help);
		} catch (parser::unknown_argument_exception const &e) {
			if (m_settings.should_exit_on_unknown_argument) {
				std::exit(1);
			}
			throw e;
		} catch (std::exception const &e) {
			if (m_settings.should_exit_on_error) {
				std::exit(1);
			}
			throw e;
		}

		check_for_required_arguments(convention_types);
		fire_on_complete_events();
	}

	void base_parser::display_help(
		const std::initializer_list<conventions::convention const *const> convention_types) const {
		std::cout << build_help_text(convention_types);
	}

	std::optional<int> base_parser::find_argument_id(std::string const &arg) const {
		const auto long_pos = long_arguments.find(arg);
		const auto short_post = short_arguments.find(arg);

		if (long_pos != long_arguments.end())
			return long_pos->second;
		if (short_post != short_arguments.end())
			return short_post->second;

		if (const auto pos_it = positional_name_map.find(arg); pos_it != positional_name_map.end())
			return pos_it->second;

		return std::nullopt;
	}

	void base_parser::assert_argument_not_exist(std::string const &short_arg, std::string const &long_arg) const {
		if (contains(short_arguments, short_arg) || contains(long_arguments, long_arg)) {
			throw std::runtime_error("The key already exists!");
		}
	}

	void base_parser::set_argument_status(const bool is_required, std::string const &help_text, argument &arg) {
		arg.set_required(is_required);
		arg.set_help_text(help_text);
	}

	void base_parser::place_argument(int const id, argument const &arg, std::string const &short_arg,
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

	void base_parser::assert_positional_not_exist(std::string const &name) const {
		if (positional_name_map.find(name) != positional_name_map.end()) {
			throw std::runtime_error("Positional argument with name '" + name + "' already exists!");
		}
	}

	void base_parser::assert_can_place_positional(const int id, const std::optional<int> position,
												  const bool accumulator) const {
		const auto existing_accumulator =
			std::find_if(positional_arguments.begin(), positional_arguments.end(), [this](int const arg_id) {
				if (arg_id == -1) {
					return false;
				}
				return argument_map.at(arg_id).is_positional_accumulator();
			});

		if (accumulator && existing_accumulator != positional_arguments.end()) {
			throw std::runtime_error("Only one positional accumulator is allowed.");
		}

		if (!accumulator && existing_accumulator != positional_arguments.end()) {
			const auto accumulator_slot =
				static_cast<size_t>(std::distance(positional_arguments.begin(), existing_accumulator));
			if (!position.has_value() || static_cast<size_t>(position.value()) >= accumulator_slot) {
				throw std::runtime_error("Positional accumulator must be the last positional argument.");
			}
		}

		if (accumulator && position.has_value()) {
			const auto idx = static_cast<size_t>(position.value());
			for (size_t i = idx + 1; i < positional_arguments.size(); ++i) {
				if (positional_arguments[i] != -1 && positional_arguments[i] != id) {
					throw std::runtime_error("Positional accumulator must be the last positional argument.");
				}
			}
		}
	}

	void base_parser::place_positional_argument(int const id, argument const &arg, std::string const &name,
												const std::optional<int> position, const bool accumulator) {
		if (position.has_value() && position.value() < 0) {
			throw std::runtime_error("Positional argument position cannot be negative.");
		}
		assert_can_place_positional(id, position, accumulator);

		argument_map[id] = arg;
		positional_name_map[name] = id;
		reverse_positional_names[id] = name;

		if (position.has_value()) {
			const auto idx = static_cast<size_t>(position.value());
			if (idx > positional_arguments.size()) {
				positional_arguments.resize(idx + 1, -1);
			}
			if (idx < positional_arguments.size() && positional_arguments[idx] != -1) {
				positional_arguments.insert(positional_arguments.begin() + static_cast<std::ptrdiff_t>(idx), id);
				return;
			}
			if (idx == positional_arguments.size()) {
				positional_arguments.push_back(id);
			} else {
				positional_arguments[idx] = id;
			}
		} else {
			if (const auto empty_slot = std::find(positional_arguments.begin(), positional_arguments.end(), -1);
				empty_slot != positional_arguments.end()) {
				*empty_slot = id;
			} else {
				positional_arguments.push_back(id);
			}
		}
	}

	std::optional<size_t> base_parser::next_positional_slot(size_t const start) const {
		for (size_t i = start; i < positional_arguments.size(); ++i) {
			if (positional_arguments[i] != -1) {
				return i;
			}
		}
		return std::nullopt;
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
		std::vector<std::tuple<std::string, std::string, bool, bool>> required_args;
		for (auto const &[key, arg] : argument_map) {
			if (arg.is_required() && !arg.is_invoked()) {
				if (arg.is_positional()) {
					auto pos_name = reverse_positional_names.find(key) != reverse_positional_names.end()
										? reverse_positional_names.at(key)
										: "unknown";
					required_args.emplace_back(pos_name, "", true, true);
				} else {
					auto short_arg = reverse_short_arguments.find(key) != reverse_short_arguments.end()
										 ? reverse_short_arguments.at(key)
										 : "-";
					auto long_arg = reverse_long_arguments.find(key) != reverse_long_arguments.end()
										? reverse_long_arguments.at(key)
										: "-";
					required_args.emplace_back(short_arg, long_arg, arg.expects_parameter(), false);
				}
			}
		}

		if (!required_args.empty()) {
			std::cerr << "These arguments were expected but not provided: \n";
			for (auto const &[s, l, p, is_pos] : required_args) {
				if (is_pos) {
					std::cerr << "\t<" << s << ">: positional argument must be provided\n";
				} else {
					std::cerr << "\t" << get_one_name(s, l) << ": must be provided as one of [";
					for (auto it = convention_types.begin(); it != convention_types.end(); ++it) {
						auto [short_part, long_part] = (*it)->make_help_text(s, l, p);
						std::string help_str = short_part;
						if (!short_part.empty() && !long_part.empty()) {
							help_str += "  ";
						}
						help_str += long_part;

						if (size_t last_not_space = help_str.find_last_not_of(" \t");
							last_not_space != std::string::npos) {
							help_str.erase(last_not_space + 1);
						}
						std::cerr << help_str;
						if (it + 1 != convention_types.end()) {
							std::cerr << ", ";
						}
					}
					std::cerr << "]\n";
				}
			}
			std::cerr << "\n";
			display_help(convention_types);
			if (m_settings.should_exit_on_missing_required) {
				std::exit(1);
			}
			throw std::runtime_error("required arguments not provided");
		}
	}

	void base_parser::fire_on_complete_events() const {
		for (auto const &event : on_complete_events) {
			event(*this);
		}
	}
} // namespace argument_parser
