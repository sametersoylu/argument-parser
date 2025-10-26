#pragma once 
#include <argument_parser.hpp>
#include <cstdlib>
#include <exception>
#include <fake_parser.hpp>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace argument_parser::v2 {
    namespace internal {
        static inline fake_parser fake_parser{}; 
    }

    enum class add_argument_flags {
        ShortArgument, 
        LongArgument,
        HelpText,
        Action, 
        Required
    };

    namespace flags {
        constexpr static inline add_argument_flags ShortArgument = add_argument_flags::ShortArgument; 
        constexpr static inline add_argument_flags LongArgument = add_argument_flags::LongArgument; 
        constexpr static inline add_argument_flags HelpText = add_argument_flags::HelpText; 
        constexpr static inline add_argument_flags Action = add_argument_flags::Action; 
        constexpr static inline add_argument_flags Required = add_argument_flags::Required; 
    }

    class base_parser : private argument_parser::base_parser {
        public:
        template<typename T> 
        using typed_flag_value = std::variant<std::string, parametered_action<T>, bool>; 
        using non_typed_flag_value = std::variant<std::string, non_parametered_action, bool>;  

        template<typename T> 
        using typed_argument_pair = std::pair<add_argument_flags, typed_flag_value<T>>; 
        using non_typed_argument_pair = std::pair<add_argument_flags, non_typed_flag_value>; 

        template<typename T>
        void add_argument(std::unordered_map<add_argument_flags, typed_flag_value<T>> const& argument_pairs) {
            std::unordered_map<extended_add_argument_flags, bool> found_params {{
                extended_add_argument_flags::IsTyped, true
            }}; 

            std::string short_arg, long_arg, help_text; 
            std::unique_ptr<action_base> action; 
            bool required = false; 

            if (argument_pairs.contains(add_argument_flags::ShortArgument)) {
                found_params[extended_add_argument_flags::ShortArgument] = true; 
                short_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::ShortArgument), "short"); 
            }
            if (argument_pairs.contains(add_argument_flags::LongArgument)) {
                found_params[extended_add_argument_flags::LongArgument] = true; 
                long_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::LongArgument), "long");
                if (short_arg.empty()) short_arg = long_arg; 
            } else {
                if (!short_arg.empty()) long_arg = short_arg; 
            }

            if (argument_pairs.contains(add_argument_flags::Action)) {
                found_params[extended_add_argument_flags::Action] = true; 
                action = get_or_throw<parametered_action<T>>(argument_pairs.at(add_argument_flags::Action), "action").clone(); 
            }
            if (argument_pairs.contains(add_argument_flags::HelpText)) {
                help_text = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::HelpText), "help"); 
            } else {
                help_text = short_arg + ", " + long_arg; 
            }
            if (argument_pairs.contains(add_argument_flags::Required) && get_or_throw<bool>(argument_pairs.at(add_argument_flags::Required), "required")) {
                required = true; 
            }

            auto suggested_add = suggest_candidate(found_params); 
            if (suggested_add == candidate_type::unknown) {
                throw std::runtime_error("Could not match any add argument overload to given parameters. Are you missing some required parameter?"); 
            }
            switch (suggested_add) {
                case candidate_type::typed_action: 
                    base::add_argument(short_arg, long_arg, help_text, *static_cast<parametered_action<T>*>(&(*action)), required); 
                    break; 
                case candidate_type::store_other: 
                    base::add_argument(short_arg, long_arg, help_text, required); 
                    break; 
                default:
                    throw std::runtime_error("Could not match the arguments against any overload."); 
            }
        }
        
        template<typename T> 
        void add_argument(std::initializer_list<typed_argument_pair<T>> const& pairs) {
            std::unordered_map<add_argument_flags, typed_flag_value<T>> args; 

            for (auto& [k, v]: pairs) {
                args[k] = v; 
            }

            add_argument<T>(args); 
        }

        void add_argument(std::initializer_list<non_typed_argument_pair> const& pairs) {
            std::unordered_map<add_argument_flags, non_typed_flag_value> args; 
            
            for (auto& [k, v] : pairs) {
                args[k] = v; 
            }

            add_argument(args); 
        }

        void add_argument(std::unordered_map<add_argument_flags, non_typed_flag_value> const& argument_pairs) {
            std::unordered_map<extended_add_argument_flags, bool> found_params {{
                extended_add_argument_flags::IsTyped, false
            }}; 

            std::string short_arg, long_arg, help_text; 
            std::unique_ptr<action_base> action;  
            bool required = false; 

            if (argument_pairs.contains(add_argument_flags::ShortArgument)) {
                found_params[extended_add_argument_flags::ShortArgument] = true; 
                short_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::ShortArgument), "short"); 
            }
            if (argument_pairs.contains(add_argument_flags::LongArgument)) {
                found_params[extended_add_argument_flags::LongArgument] = true; 
                long_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::LongArgument), "long");
                if (short_arg.empty()) short_arg = long_arg; 
            } else {
                if (!short_arg.empty()) long_arg = short_arg; 
            }
            if (argument_pairs.contains(add_argument_flags::Action)) {
                found_params[extended_add_argument_flags::Action] = true; 
                action = get_or_throw<non_parametered_action>(argument_pairs.at(add_argument_flags::Action), "action").clone();
            
            }
            if (argument_pairs.contains(add_argument_flags::HelpText)) {
                help_text = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::HelpText), "help"); 
            } else {
                help_text = short_arg + ", " + long_arg; 
            }

            if (argument_pairs.contains(add_argument_flags::Required) && get_or_throw<bool>(argument_pairs.at(add_argument_flags::Required), "required")) {
                required = true; 
            }

            auto suggested_add = suggest_candidate(found_params); 
            if (suggested_add == candidate_type::unknown) {
                throw std::runtime_error("Could not match any add argument overload to given parameters. Are you missing some required parameter?"); 
            }

            switch (suggested_add) {
                case candidate_type::non_typed_action: 
                    base::add_argument(short_arg, long_arg, help_text, *static_cast<non_parametered_action*>(&(*action)), required); 
                    break; 
                case candidate_type::store_boolean: 
                    base::add_argument(short_arg, long_arg, help_text, required); 
                    break; 
                default:
                    throw std::runtime_error("Could not match the arguments against any overload. The suggested candidate was: " + std::to_string((int(suggested_add)))); 
            }
        }

        argument_parser::base_parser& to_v1() {
            return *this; 
        }

        void handle_arguments(std::initializer_list<conventions::convention const* const> convention_types) {
            base::handle_arguments(convention_types); 
        }

        template<typename T>
        std::optional<T> get_optional(std::string const& arg) {
            return base::get_optional<T>(arg); 
        }

        void on_complete(std::function<void(argument_parser::base_parser const&)> const& action) {
            base::on_complete(action); 
        }

        protected:
        void set_program_name(std::string p) {
            base::program_name = std::move(p); 
        }

        std::vector<std::string>& ref_parsed_args() {
            return base::parsed_arguments; 
        }

        private: 
        using base = argument_parser::base_parser; 
        enum class extended_add_argument_flags {
            ShortArgument, 
            LongArgument,
            Action, 
            IsTyped
        }; 

        enum class candidate_type {
            store_boolean,
            store_other,
            typed_action,
            non_typed_action, 
            unknown
        };

        template<typename T, size_t S> 
        bool satisfies_at_least_one(std::array<T, S> const& arr, std::unordered_map<T, bool> const& map) {
            for (const auto& req: arr) {
                if (map.contains(req)) return true; 
            }
            return false; 
        }

        candidate_type suggest_candidate(std::unordered_map<extended_add_argument_flags, bool> const& available_vars) {
            auto constexpr required_at_least_one = std::array<extended_add_argument_flags, 2> { extended_add_argument_flags::ShortArgument, extended_add_argument_flags::LongArgument }; 
            if (!satisfies_at_least_one(required_at_least_one, available_vars)) return candidate_type::unknown;
            
            if (available_vars.contains(extended_add_argument_flags::Action)) { 
                if (available_vars.at(extended_add_argument_flags::IsTyped))
                    return candidate_type::typed_action;
                else return candidate_type::non_typed_action; 
            }
            
            if (available_vars.at(extended_add_argument_flags::IsTyped)) return candidate_type::store_other; 
            return candidate_type::store_boolean; 
        }

        template<typename T, typename I> 
        T get_or_throw(typed_flag_value<I> const& v, std::string_view key) {
            if (auto p = std::get_if<T>(&v)) return *p;
            throw std::invalid_argument(std::string("variant type mismatch for key: ") + std::string(key));
        }

        template<typename T> 
        T get_or_throw(non_typed_flag_value const& v, std::string_view key) {
            if (auto p = std::get_if<T>(&v)) return *p;
            throw std::invalid_argument(std::string("variant type mismatch for key: ") + std::string(key));
        }
    }; 
}