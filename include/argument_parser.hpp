#pragma once
#include "base_convention.hpp"
#include <atomic>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include <type_traits>
#include <string>

namespace argument_parser {
    namespace function {
        template<typename fn>
        concept no_param = std::is_invocable_r_v<void, fn>;

        template<typename fn, typename param_type> 
        concept with_param = std::is_invocable_r_v<void, fn, param_type const&>; 

        template<typename fn>
        concept string_parameter = with_param<fn, std::string>;
        
        template<typename fn, typename param_type> 
        concept integral_parameter = with_param<fn, param_type> && std::is_integral_v<param_type>; 

        template<typename T_>
        using parametered_action = std::function<void(T_ const&)>;

        using non_parametered_action = std::function<void()>; 
    }

    namespace type {
        enum class parameter_type {
            NONE,
            INT,
            FLOAT,
            BOOL,
            STRING
        }; 

        template<typename T>
        struct param_type_map;

        template<> struct param_type_map<int>    { static constexpr parameter_type value = parameter_type::INT; };
        template<> struct param_type_map<float>  { static constexpr parameter_type value = parameter_type::FLOAT; };
        template<> struct param_type_map<bool>   { static constexpr parameter_type value = parameter_type::BOOL; };
        template<> struct param_type_map<std::string> { static constexpr parameter_type value = parameter_type::STRING; };

        template<typename T>
        concept supported_parameter_type = requires {
            { param_type_map<T>::value } -> std::convertible_to<parameter_type>;
        };
    }

    namespace error {
        enum class type {
            missing_parameter,
            key_redefined
        };

        constexpr type missing_parameter = type::missing_parameter; 
        constexpr type key_redefined = type::key_redefined; 
    }

    template <typename T_> class parametered_action;
    class non_parametered_action;

    class base_action {}; 

    template<typename child>
    class action_impl : public base_action {
        public: 
        void invoke() {
            static_cast<child*>(this)->invoke_impl(); 
        } 

        template<typename T_> 
        void invoke(T_ const& var) {
            if (!validate_type(parametered_action<T_>{})) 
                throw std::runtime_error("this action does not take any parameter, or given parameters do not match the required parameter type."); 
            static_cast<child*>(this)->invoke_impl(var);
        }  
        
        std::type_info const& get_type() const {
            return static_cast<child const*>(this)->get_type_impl(); 
        }

        template<typename T>
        bool validate_type(action_impl<T> const& other) {
            return get_type() == other.get_type();  
        }
    }; 
    
    template <typename T_> 
    class parametered_action : public action_impl<parametered_action<T_>> {
        public: 
        parametered_action() {}
        parametered_action(function::parametered_action<T_> const& function) : handler(function) {}
        template<typename T>
        static std::type_info const& type() {
            return typeid(parametered_action<T>); 
        }

        std::type_info const& get_type_impl() const {
            return type<T_>();        
        }
        private:
        function::parametered_action<T_> handler; 
        template<typename arg_type>
        void invoke_impl(arg_type const& arg) {
            handler(arg); 
        }

        friend class action_impl<parametered_action<T_>>; 
    }; 
    
    class non_parametered_action : public action_impl<non_parametered_action> {
        public: 
        non_parametered_action(std::function<void()> const& function) : handler(function) {}
        static std::type_info const& type() {
            return typeid(non_parametered_action); 
        }

        std::type_info const& get_type_impl() const {
            return type();        
        }
        private: 
        std::function<void()> handler; 
        void invoke_impl() {
            this->handler(); 
        }

        friend class action_impl<non_parametered_action>; 
    };

    class helpers {
        public: 
        template<typename T_>
        static std::shared_ptr<parametered_action<T_>> make_parametered_action_ptr(function::parametered_action<T_> const& function) {
            return std::make_shared<parametered_action<T_>>(parametered_action<T_>(function));
        }
        
        template<typename T_> 
        static parametered_action<T_> make_parametered_action(function::parametered_action<T_> const& function) {
            return parametered_action<T_>(function); 
        }

        static non_parametered_action make_non_parametered_action(function::non_parametered_action const& function) {
            return non_parametered_action(function); 
        }

        static std::shared_ptr<non_parametered_action> make_non_parametered_action_ptr(function::non_parametered_action const& function) {
            return std::make_shared<non_parametered_action>(non_parametered_action(function)); 
        }
    };

    class argument {
        using parameter_type = type::parameter_type; 
        public:
        argument() : name(), id(0) {}
        argument(std::string const& name, int id) : name(name), id(id) {}

        // parametered action group
        template<type::supported_parameter_type T_>
        argument(int id, std::string const& name, parametered_action<T_> const& action) : argument(name, id) {
            type = extract_type<T_>(); 
            this->action = std::make_shared<parametered_action<T_>>(action);
        }

        template<type::supported_parameter_type T_>
        argument(int id, std::string const& name, std::shared_ptr<parametered_action<T_>> const& action) : argument(name, id) {
            type = extract_type<T_>(); 
            this->action = action; 
        }

        template<type::supported_parameter_type T_>
        argument(std::string const& name, parametered_action<T_> const& action) : argument(name, 0) {
            type = extract_type<T_>(); 
            this->action = std::make_shared<parametered_action<T_>>(action);
        }

        template<type::supported_parameter_type T_>
        argument(std::string const& name, std::shared_ptr<parametered_action<T_>> const& action) : argument(name, 0) {
            type = extract_type<T_>(); 
            this->action = action; 
        }

        // non parametered action group 
        argument(int id, std::string const& name, non_parametered_action const& action) : argument(name, id) {
            type = parameter_type::NONE;
            this->action = std::make_shared<non_parametered_action>(action);
        }

        argument(int id, std::string const& name, std::shared_ptr<non_parametered_action> const& action) : argument(name, id) {
            type = parameter_type::NONE; 
            this->action = action; 
        }

        argument(std::string const& name, non_parametered_action const& action) : argument(name, 0) {
            type = parameter_type::NONE;
            this->action = std::make_shared<non_parametered_action>(action);
        }

        argument(std::string const& name, std::shared_ptr<non_parametered_action> const& action) : argument(name, 0) {
            type = parameter_type::NONE;
            this->action = action; 
        }

        argument(argument const& other) : 
            id(other.id), 
            name(other.name), 
            action(other.action), 
            required(other.required), 
            type(other.type),
            help_text(other.help_text),
            invoked(other.invoked) 
        {}
        argument& operator=(argument const& other) {
            if (this == &other) return *this; 
            this->~argument(); 
            new(this) argument{other};
            return *this; 
        }
        
        void toggle_required() {
            required = !required; 
        }


        bool is_required() const {
            return required; 
        } 

        std::string get_name() const {
            return name; 
        }

        parameter_type expected_type() const {
            return type; 
        }

        bool expects_parameter() const {
            return type != parameter_type::NONE; 
        }

        bool is_invoked() const {
            return invoked; 
        }

        template<typename T_> 
        void invoke(T_ const& argument) {
            if (type == parameter_type::NONE) throw std::runtime_error("this argument does not expect any parameter.");
            invoked = true; 
            static_cast<action_impl<parametered_action<T_>>*>(action.get())->invoke(argument); 
        }

        void invoke() {
            if (type != parameter_type::NONE) throw std::runtime_error("this argument expects parameter."); 
            invoked = true; 
            static_cast<action_impl<non_parametered_action>*>(action.get())->invoke();
        }

        private: 
        void set_required(bool val) {
            required = val; 
        }

        void set_help_text(std::string const& help_text) {
            this->help_text = help_text; 
        }
        argument(type::parameter_type const& parameter_type) : id(0), name(), type(parameter_type) {}
        friend class base_parser; 
        template<type::supported_parameter_type T_>
        constexpr type::parameter_type extract_type() {
            if constexpr (std::is_same_v<T_, int>)
                return type::parameter_type::INT;
            else if constexpr (std::is_same_v<T_, float> || std::is_convertible_v<T_, float>)
                return type::parameter_type::FLOAT;
            else if constexpr (std::is_same_v<T_, bool>)
                return type::parameter_type::BOOL;
            else if constexpr (std::is_same_v<T_, std::string> || std::is_convertible_v<T_, std::string>)
                return type::parameter_type::STRING;
        }


        int const id;
        std::string const name;
        std::shared_ptr<base_action> action;
        bool required = false; 
        parameter_type type;
        bool invoked = false; 
        std::string help_text; 
    }; 

    class base_parser { 
        public: 
        template<typename T_>
        void add_argument(std::string const& short_arg, std::string const& long_arg, parametered_action<T_> const& action) {
            base_add_argument(short_arg, long_arg, long_arg, action, false); 
        }

        template<typename T_>
        void add_argument(std::string const& short_arg, std::string const& long_arg, parametered_action<T_> const& action, bool required) {
            base_add_argument(short_arg, long_arg, long_arg, action, required); 
        }

        template<typename T_>
        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, parametered_action<T_> const& action, bool required) {
            base_add_argument(short_arg, long_arg, help_text, action, required); 
        }

        void add_argument(std::string const& short_arg, std::string const& long_arg, non_parametered_action const& action) {
            base_add_argument(short_arg, long_arg, long_arg, action, false); 
        }

        void add_argument(std::string const& short_arg, std::string const& long_arg, non_parametered_action const& action, bool required) {
            base_add_argument(short_arg, long_arg, long_arg, action, required); 
        }

        void add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, non_parametered_action const& action, bool required) {
            base_add_argument(short_arg, long_arg, help_text, action, required); 
        }

        void handle_arguments(std::initializer_list<conventions::convention const* const> convention_types) {
            for (auto it = parsed_arguments.begin(); it != parsed_arguments.end(); ++it) {
                std::stringstream ss;  
                bool arg_correctly_handled = false; 
                for(auto const& convention_type : convention_types) {
                    auto extracted = convention_type->get_argument(*it); 
                    if (extracted.first == conventions::argument_type::ERROR) {
                        ss << "Convention \"" << convention_type->name() << "\" failed with: " << extracted.second << "\n";
                        continue;
                    }
                    
                    try {
                        argument& coresponding_argument = get_argument(extracted);
                        if (coresponding_argument.expects_parameter()) {
                            auto type = coresponding_argument.expected_type();
                            if (convention_type->requires_next_token() and (it + 1) == parsed_arguments.end()) 
                                throw std::runtime_error("expected value for argument " + extracted.second + " argument"); 
                            auto value_raw = convention_type->requires_next_token() ? *(++it) : convention_type->extract_value(*it); 
                            if (type == type::parameter_type::BOOL) {
                                auto value = parse_bool(value_raw);
                                coresponding_argument.invoke<bool>(value); 
                                arg_correctly_handled = true;
                                break; 
                            }
                            else if (type == type::parameter_type::INT) {
                                auto value = parse_int(value_raw); 
                                coresponding_argument.invoke<int>(value); 
                                arg_correctly_handled = true;
                                break; 
                            } 
                            else if (type == type::parameter_type::FLOAT) {
                                auto value = parse_float(value_raw); 
                                coresponding_argument.invoke<float>(value); 
                                arg_correctly_handled = true;
                                break; 
                            } 
                            else if (type == type::parameter_type::STRING) {
                                coresponding_argument.invoke<std::string>(value_raw); 
                                arg_correctly_handled = true; 
                                break; 
                            }
                        }
                        else {
                            coresponding_argument.invoke();
                            arg_correctly_handled = true;  
                            break;
                        }    
                    } catch(std::runtime_error e) {
                        ss << "Convention \"" << convention_type->name() << "\" failed with: " << e.what() << "\n"; 
                        continue;
                    }
                }
                if (!arg_correctly_handled) {
                    throw std::runtime_error("All trials for argument: \n\t\"" + *it + "\"\n failed with: \n" + ss.str());
                }
            }

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

        void display_help(std::initializer_list<conventions::convention const* const> convention_types) {
            std::cout << build_help_text(convention_types); 
        }

        std::string build_help_text(std::initializer_list<conventions::convention const* const> convention_types) {
            std::stringstream ss;
            
            ss << "Usage: " << program_name
                    << " [OPTIONS]... [ARGS]...\n"; 
                
            
            for (auto  const& [id, arg] : argument_map) {
                auto short_arg = reverse_short_arguments[id];
                auto long_arg = reverse_long_arguments[id]; 
                ss << "\t";
                for (auto const& convention : convention_types) {
                    ss << convention->short_prec() << short_arg << ", ";  
                }
                bool first = true; 
                for (auto const& convention : convention_types) {
                    ss << (first ? "" : ", ") << convention->long_prec() << long_arg;  
                    first = false; 
                }

                ss << "\t\t" << arg.help_text << "\n"; 
            }

            return ss.str(); 
        }

        private:
        bool parse_bool(std::string const& val) {
            if (val == "t" || val == "true" || val == "1") return true; 
            if (val == "f" || val == "false" || val == "0") return false; 
            throw std::runtime_error("expected boolean parsable, got:" + val); 
        }

        int parse_int(std::string const& val) {
            return std::stoi(val); 
        }

        int parse_float(std::string const& val) {
            return std::stof(val); 
        }

        base_parser() : argument_map(), short_arguments(), long_arguments(), reverse_long_arguments(), reverse_short_arguments(), parsed_arguments(), program_name() {}

        template<typename T_> 
        void base_add_argument(std::string const& short_arg, std::string const& long_arg, std::string const& help_text, T_ const& action, bool required) {
            if (short_arguments.find(short_arg) != short_arguments.end() || long_arguments.find(long_arg) != long_arguments.end()) {
                throw std::runtime_error("The key already exists!"); 
            }

            int id = __id_ref.fetch_add(1);
            
            auto arg = argument(id, short_arg + "|" + long_arg, action);;
            arg.set_required(required); 
            arg.set_help_text(help_text);

            argument_map[id] = arg;  
            short_arguments[short_arg] = id;
            reverse_short_arguments[id] = short_arg; 
            long_arguments[long_arg] = id;  
            reverse_long_arguments[id] = long_arg; 
        }

        public: 
        argument& get_argument(conventions::parsed_argument const& arg) {
            if (arg.first == conventions::argument_type::LONG) {
                auto long_pos = long_arguments.find(arg.second);
                if (long_pos != long_arguments.end()) return argument_map[long_pos->second]; 
            } else if (arg.first == conventions::argument_type::SHORT) {
                auto short_pos = short_arguments.find(arg.second); 
                if (short_pos != short_arguments.end()) return argument_map[short_pos->second];
            } else if (arg.first == conventions::argument_type::ERROR) {
                throw std::runtime_error{arg.second}; 
            }
            throw std::runtime_error("Unknown argument: " + arg.second);
        }

        private:
        std::string program_name;

        inline static std::atomic_int __id_ref = std::atomic_int(0);

        std::unordered_map<int, argument> argument_map; 
        std::unordered_map<std::string, int> short_arguments; 
        std::unordered_map<int, std::string> reverse_short_arguments; 
        std::unordered_map<std::string, int> long_arguments; 
        std::unordered_map<int, std::string> reverse_long_arguments; 

        std::vector<std::string> parsed_arguments; 

        friend class linux_parser; 
        friend class windows_parser; 
    };
}

#endif // ARGUMENT_PARSER_HPP
