#pragma once
#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include <traits.hpp>
#include <base_convention.hpp>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <variant> 

namespace argument_parser {
    template <typename T>
    class parametered_action {
    public:
        // Type alias to expose the parameter type, crucial for the visitor.
        using parameter_type = T;
        
        parametered_action(std::function<void(const T&)> const& function) : handler(function) {}

        void invoke(const T& arg) const {
            handler(arg);
        }

    private:
        std::function<void(const T&)> handler;
    };

    class non_parametered_action {
    public:
        non_parametered_action(std::function<void()> const& function) : handler(function) {}
        
        void invoke() const {
            handler();
        }

    private:
        std::function<void()> handler;
    };

    using action_variant = std::variant<
        non_parametered_action,
        parametered_action<std::string>,
        parametered_action<int>,
        parametered_action<float>,
        parametered_action<bool>
    >;

    class base_parser;

    class argument {
    public:
        argument() : id(0), name(), required(false), invoked(false), action(non_parametered_action([](){})) {}

        template <typename ActionType>
        argument(int id, std::string const& name, ActionType const& action) 
            : id(id), name(name), action(action), required(false), invoked(false) {}

        bool is_required() const { return required; }
        std::string get_name() const { return name; }
        bool is_invoked() const { return invoked; }

        bool expects_parameter() const {
            return !std::holds_alternative<non_parametered_action>(action);
        }

    private:
        void set_required(bool val) { required = val; }
        void set_invoked(bool val) { invoked = val; }
        void set_help_text(std::string const& text) { help_text = text; }

        friend class base_parser;

        int id;
        std::string name;
        action_variant action;
        bool required;
        bool invoked;
        std::string help_text;
    };

    class base_parser {
    public:
        template <typename T>
        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, parametered_action<T> const& action, bool required) {
            base_add_argument(short_arg, long_arg, help_text, action, required);
        }

        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, non_parametered_action const& action, bool required) {
            base_add_argument(short_arg, long_arg, help_text, action, required);
        }

        std::string build_help_text(std::initializer_list<conventions::convention const* const> convention_types) {
            std::stringstream ss;
            ss << "Usage: " << program_name << " [OPTIONS]...\n";
            
            for (auto const& [id, arg] : argument_map) {
                auto short_arg = reverse_short_arguments[id];
                auto long_arg = reverse_long_arguments[id];
                ss << "\t";
                ss << "-" << short_arg << ", --" << long_arg;
                ss << "\t\t" << arg.help_text << "\n";
            }
            return ss.str();
        }

        argument& get_argument(conventions::parsed_argument const& arg) {
            if (arg.first == conventions::argument_type::LONG) {
                auto long_pos = long_arguments.find(arg.second);
                if (long_pos != long_arguments.end()) return argument_map.at(long_pos->second);
            } else if (arg.first == conventions::argument_type::SHORT) {
                auto short_pos = short_arguments.find(arg.second);
                if (short_pos != short_arguments.end()) return argument_map.at(short_pos->second);
            }
            throw std::runtime_error("Unknown argument: " + arg.second);
        }
        
        void handle_arguments(std::initializer_list<conventions::convention const* const> convention_types) {
            for (auto it = parsed_arguments.begin(); it != parsed_arguments.end(); ++it) {
                std::stringstream error_stream;
                bool arg_correctly_handled = false;

                for (auto const& convention_type : convention_types) {
                    auto extracted = convention_type->get_argument(*it);
                    if (extracted.first == conventions::argument_type::ERROR) {
                        error_stream << "Convention \"" << convention_type->name() << "\" failed with: " << extracted.second << "\n";
                        continue;
                    }

                    try {
                        argument& corresponding_argument = get_argument(extracted);
                        
                        std::visit([&](auto&& action) {
                            using ActionType = std::decay_t<decltype(action)>;
                            
                            if constexpr (std::is_same_v<ActionType, non_parametered_action>) {
                                action.invoke();
                            } else {
                                if (convention_type->requires_next_token() && (it + 1) == parsed_arguments.end()) {
                                    throw std::runtime_error("expected value for argument " + extracted.second);
                                }
                                auto value_raw = convention_type->requires_next_token() ? *(++it) : convention_type->extract_value(*it);
                                
                                using ParamType = typename ActionType::parameter_type;
                                
                                auto value = parsing_traits::parser_trait<ParamType>::parse(value_raw);
                                
                                action.invoke(value);
                            }
                        }, corresponding_argument.action);

                        corresponding_argument.set_invoked(true);
                        arg_correctly_handled = true;
                        break; // Convention succeeded, move to the next argument token

                    } catch (const std::runtime_error& e) {
                        error_stream << "Convention \"" << convention_type->name() << "\" failed with: " << e.what() << "\n";
                    }
                }

                if (!arg_correctly_handled) {
                    throw std::runtime_error("All trials for argument: \n\t\"" + *it + "\"\n failed with: \n" + error_stream.str());
                }
            }
            check_for_required_arguments(convention_types);
        }

        void display_help(std::initializer_list<conventions::convention const* const> convention_types) {
            std::cout << build_help_text(convention_types);
        }

    protected:
        base_parser() = default;

        std::string program_name;
        std::vector<std::string> parsed_arguments;

    private:
        template <typename ActionType>
        void base_add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, ActionType const& action, bool required) {
            if (short_arguments.count(short_arg) || long_arguments.count(long_arg)) {
                throw std::runtime_error("The key already exists!");
            }

            int id = id_counter.fetch_add(1);

            argument arg(id, short_arg + "|" + long_arg, action);
            arg.set_required(required);
            arg.set_help_text(help_text);

            argument_map[id] = arg;
            short_arguments[short_arg] = id;
            reverse_short_arguments[id] = short_arg;
            long_arguments[long_arg] = id;
            reverse_long_arguments[id] = long_arg;
        }

        void check_for_required_arguments(std::initializer_list<conventions::convention const* const> convention_types) {
            std::vector<std::pair<std::string, std::string>> required_args; 
            for (auto const& [_, arg] : argument_map) {
                if (arg.is_required() and not arg.is_invoked()) {
                    required_args.emplace_back<std::pair<std::string, std::string>>({
                        reverse_short_arguments[arg.id],
                        reverse_long_arguments[arg.id]
                    });
                } 
            }

            if (not required_args.empty()) {
                std::cerr << "These arguments were expected but not provided: "; 
                for (auto const& [s, l] : required_args) {
                    std::cerr << "[-" << s << ", --" << l << "] ";
                }
                std::cerr << "\n"; 
                display_help(convention_types); 
            }
        }

        inline static std::atomic_int id_counter = 0;
        
        std::unordered_map<int, argument> argument_map;
        std::unordered_map<std::string, int> short_arguments;
        std::unordered_map<int, std::string> reverse_short_arguments;
        std::unordered_map<std::string, int> long_arguments;
        std::unordered_map<int, std::string> reverse_long_arguments;

        friend class linux_parser;
        friend class windows_parser;
        friend class macos_parser;
        friend class fake_parser;
    };

    namespace helpers {
        template<typename T>
        static parametered_action<T> make_parametered_action(std::function<void(const T&)> const& function) {
            return parametered_action<T>(function);
        }

        static non_parametered_action make_non_parametered_action(std::function<void()> const& function) {
            return non_parametered_action(function);
        }
    }
}

#endif // ARGUMENT_PARSER_HPP