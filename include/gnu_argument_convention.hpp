#pragma once
#include "base_convention.hpp"
#include <stdexcept>

#ifndef GNU_ARGUMENT_CONVENTION_HPP
#define GNU_ARGUMENT_CONVENTION_HPP

namespace argument_parser::conventions::implementations {

    class gnu_argument_convention : public base_convention {
    public:
        parsed_argument get_argument(std::string const& raw) const override {
            if (raw.starts_with(long_prec()))
                return {argument_type::LONG, raw.substr(2)};
            else if (raw.starts_with(short_prec()))
                return {argument_type::SHORT, raw.substr(1)};
            else
                return {argument_type::ERROR, "GNU standard convention does not allow arguments without a preceding dash."};
        }

        std::string extract_value(std::string const& /*raw*/) const override {
            // In non-equal GNU, value comes in next token
            throw std::runtime_error("No inline value in standard GNU convention.");
        }

        bool requires_next_token() const override {
            return true; 
        }

        std::string name() const override {
            return "GNU-style long options"; 
        }

        std::string short_prec() const override {
            return "-"; 
        }

        std::string long_prec() const override {
            return "--"; 
        }

        static gnu_argument_convention instance; 
    private: 
        gnu_argument_convention() = default; 
    };

    class gnu_equal_argument_convention : public base_convention {
    public:
        parsed_argument get_argument(std::string const& raw) const override {
            auto pos = raw.find('=');
            auto arg = pos != std::string::npos ? raw.substr(0, pos) : raw;
            if (arg.starts_with(long_prec()))
                return {argument_type::LONG, arg.substr(2) };
            else if (arg.starts_with(short_prec()))
                return {argument_type::SHORT, arg.substr(1) };
            else
                return {argument_type::ERROR, "GNU standard convention does not allow arguments without a preceding dash."};
        }

        std::string extract_value(std::string const& raw) const override {
            auto pos = raw.find('=');
            if (pos == std::string::npos || pos + 1 >= raw.size())
                throw std::runtime_error("Expected value after '='.");
            return raw.substr(pos + 1);
        }

        bool requires_next_token() const override {
            return false; 
        }

        std::string name() const override {
            return "GNU-style long options (equal signed form)"; 
        }

         std::string short_prec() const override {
            return "-"; 
        }

        std::string long_prec() const override {
            return "--"; 
        }

        static gnu_equal_argument_convention instance; 
    private:
        gnu_equal_argument_convention() = default; 
    };

    inline gnu_argument_convention gnu_argument_convention::instance{};
    inline gnu_equal_argument_convention gnu_equal_argument_convention::instance{};
}

namespace argument_parser::conventions {
    static inline const implementations::gnu_argument_convention gnu_argument_convention = implementations::gnu_argument_convention::instance; 
    static inline const implementations::gnu_equal_argument_convention gnu_equal_argument_convention = implementations::gnu_equal_argument_convention::instance;
}


#endif