#pragma once
#include <list>
#include <optional>
#include <type_traits>
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
#include <unordered_map>
#include <vector>
#include <any> 

namespace argument_parser {
    class action_base {
    public:
        virtual ~action_base() = default;
        virtual bool expects_parameter() const = 0;
        virtual void invoke() const = 0;
        virtual void invoke_with_parameter(const std::string& param) const = 0;
        virtual std::unique_ptr<action_base> clone() const = 0;
    };

    template <typename T>
    class parametered_action : public action_base {
    public:
        explicit parametered_action(std::function<void(const T&)> const& handler) : handler(handler) {}
        
        using parameter_type = T;
        
        void invoke(const T& arg) const {
            handler(arg);
        }

        bool expects_parameter() const override { return true; }
        
        void invoke() const override {
            throw std::runtime_error("Parametered action requires a parameter");
        }
        
        void invoke_with_parameter(const std::string& param) const override {
            T parsed_value = parsing_traits::parser_trait<T>::parse(param);
            invoke(parsed_value);
        }
        
        std::unique_ptr<action_base> clone() const override {
            return std::make_unique<parametered_action<T>>(handler);
        }

    private:
        std::function<void(const T&)> handler;
    };

    class non_parametered_action : public action_base {
    public:
        explicit non_parametered_action(std::function<void()> const& handler) : handler(handler) {}
        
        void invoke() const override {
            handler();
        }

        bool expects_parameter() const override { return false; }
        
        void invoke_with_parameter(const std::string& param) const override {
            invoke();
        }
        
        std::unique_ptr<action_base> clone() const override {
            return std::make_unique<non_parametered_action>(handler);
        }

    private:
        std::function<void()> handler;
    };

    class base_parser;

    class argument {
    public:
        argument() : id(0), name(), required(false), invoked(false), action(std::make_unique<non_parametered_action>([](){})) {}

        template <typename ActionType>
        argument(int id, std::string const& name, ActionType const& action) 
            : id(id), name(name), action(action.clone()), required(false), invoked(false) {}

        argument(const argument& other) 
            : id(other.id), name(other.name), action(other.action->clone()), 
              required(other.required), invoked(other.invoked), help_text(other.help_text) {}

        argument& operator=(const argument& other) {
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

        argument(argument&& other) noexcept = default;
        argument& operator=(argument&& other) noexcept = default;

        bool is_required() const { return required; }
        std::string get_name() const { return name; }
        bool is_invoked() const { return invoked; }

        bool expects_parameter() const {
            return action->expects_parameter();
        }

    private:
        void set_required(bool val) { required = val; }
        void set_invoked(bool val) { invoked = val; }
        void set_help_text(std::string const& text) { help_text = text; }

        friend class base_parser;

        int id;
        std::string name;
        std::unique_ptr<action_base> action;
        bool required;
        bool invoked;
        std::string help_text;
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

    class base_parser {
    public:
        template <typename T>
        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, parametered_action<T> const& action, bool required) {
            base_add_argument(short_arg, long_arg, help_text, action, required);
        }

        template<typename T>
        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, bool required) {
            base_add_argument<T>(short_arg, long_arg, help_text, required);
        }

        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, non_parametered_action const& action, bool required) {
            base_add_argument(short_arg, long_arg, help_text, action, required);
        }

        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, bool required) {
            base_add_argument<void>(short_arg, long_arg, help_text, required);
        }

        void on_complete(std::function<void(base_parser const&)> const& action) {
            on_complete_events.emplace_back(action); 
        }

        template<typename T>
        std::optional<T> get_optional(std::string const& arg) const {
            auto id = find_argument_id(arg); 
            if (id.has_value()) {
                auto value =  stored_arguments.find(id.value());
                if (value != stored_arguments.end() && value->second.has_value()) return std::any_cast<T>(value->second);
            }
            return std::nullopt;
        }

        std::string build_help_text(std::initializer_list<conventions::convention const* const> convention_types) const {
            std::stringstream ss;
            ss << "Usage: " << program_name << " [OPTIONS]...\n";
            
            for (auto const& [id, arg] : argument_map) {
                auto short_arg = reverse_short_arguments.at(id);
                auto long_arg = reverse_long_arguments.at(id);
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

        std::optional<int> find_argument_id(std::string const& arg) const {
            auto long_pos = long_arguments.find(arg); 
            auto short_post = short_arguments.find(arg); 

            if (long_pos != long_arguments.end()) return long_pos->second;
            if (short_post != short_arguments.end()) return short_post->second;
            return std::nullopt;
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
                        
                        if (corresponding_argument.expects_parameter()) {
                            if (convention_type->requires_next_token() && (it + 1) == parsed_arguments.end()) {
                                throw std::runtime_error("expected value for argument " + extracted.second);
                            }
                            auto value_raw = convention_type->requires_next_token() ? *(++it) : convention_type->extract_value(*it);
                            corresponding_argument.action->invoke_with_parameter(value_raw);
                        } else {
                            corresponding_argument.action->invoke();
                        }

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
            fire_on_complete_events(); 
        }

        void display_help(std::initializer_list<conventions::convention const* const> convention_types) const {
            std::cout << build_help_text(convention_types);
        }

    protected:
        base_parser() = default;

        std::string program_name;
        std::vector<std::string> parsed_arguments;

    private:
        void assert_argument_not_exist(std::string const& short_arg, std::string const& long_arg) {
            if (short_arguments.count(short_arg) || long_arguments.count(long_arg)) {
                throw std::runtime_error("The key already exists!");
            }
        }

        void set_argument_status(bool is_required, std::string const& help_text, argument& arg) {
            arg.set_required(is_required);
            arg.set_help_text(help_text);
        }
    
        void place_argument(int id, argument const& arg, std::string const& short_arg, std::string const& long_arg) {
            argument_map[id] = arg;
            short_arguments[short_arg] = id;
            reverse_short_arguments[id] = short_arg;
            long_arguments[long_arg] = id;
            reverse_long_arguments[id] = long_arg;
        }
        
        template <typename ActionType>
        void base_add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, ActionType const& action, bool required) {
            assert_argument_not_exist(short_arg, long_arg);
            int id = id_counter.fetch_add(1);
            argument arg(id, short_arg + "|" + long_arg, action);
            set_argument_status(required, help_text, arg);
            place_argument(id, arg, short_arg, long_arg);
        }

        template<typename StoreType = void>
        void base_add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, bool required) {
            assert_argument_not_exist(short_arg, long_arg);
            int id = id_counter.fetch_add(1);
            if constexpr (std::is_same_v<StoreType, void>) {
                auto action = helpers::make_non_parametered_action([id, this] { stored_arguments[id] = std::any{ true }; });
                argument arg(id, short_arg + "|" + long_arg, action);
                set_argument_status(required, help_text, arg);
                place_argument(id, arg, short_arg, long_arg);
            } else {
                auto action = helpers::make_parametered_action<StoreType>([id, this](StoreType const& value) { stored_arguments[id] = std::any{ value }; });
                argument arg(id, short_arg + "|" + long_arg, action);
                set_argument_status(required, help_text, arg);
                place_argument(id, arg, short_arg, long_arg);
            }
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

        void fire_on_complete_events() {
            for(auto const& event : on_complete_events) {
                event(*this); 
            }
        }

        inline static std::atomic_int id_counter = 0;
        
        std::unordered_map<int, std::any> stored_arguments;
        std::unordered_map<int, argument> argument_map;
        std::unordered_map<std::string, int> short_arguments;
        std::unordered_map<int, std::string> reverse_short_arguments;
        std::unordered_map<std::string, int> long_arguments;
        std::unordered_map<int, std::string> reverse_long_arguments;

        std::list<std::function<void(base_parser const&)>> on_complete_events; 

        friend class linux_parser;
        friend class windows_parser;
        friend class macos_parser;
        friend class fake_parser;
    };

    
}

#endif // ARGUMENT_PARSER_HPP