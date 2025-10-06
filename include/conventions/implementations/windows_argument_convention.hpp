#pragma once
#include "base_convention.hpp"
#include <stdexcept>

#ifndef WINDOWS_ARGUMENT_CONVENTION_HPP
#define WINDOWS_ARGUMENT_CONVENTION_HPP

namespace argument_parser::conventions::implementations {
    class windows_argument_convention : public base_convention {
    public:
		// Windows style options are prefixed with a slash ('/'). And there's no difference for short vs long options.
        parsed_argument get_argument(std::string const& raw) const override {
            if (raw.starts_with(long_prec()))
                return { argument_type::INTERCHANGABLE, raw.substr(1) };
            else
                return { argument_type::ERROR, "Windows standard requires non-positional arguments with a preceeding slash ('\\')." };
        }

        std::string extract_value(std::string const& /*raw*/) const override {
            // In non-equal GNU, value comes in next token
            throw std::runtime_error("No inline value in standard GNU convention.");
        }

        bool requires_next_token() const override {
            return true;
        }

        std::string name() const override {
            return "Windows style options";
        }

        std::string short_prec() const override {
            return "/";
        }

        std::string long_prec() const override {
            return "/";
        }

        static windows_argument_convention instance;
    private:
        windows_argument_convention() = default;
    };

    class windows_equal_argument_convention : public base_convention {
    public:
        // Windows style options are prefixed with a slash ('/'). And there's no difference for short vs long options.
        parsed_argument get_argument(std::string const& raw) const override {
            auto pos = raw.find('=');
            auto arg = pos != std::string::npos ? raw.substr(0, pos) : raw;
            if (arg.starts_with(long_prec()))
                return { argument_type::INTERCHANGABLE, arg.substr(1) };
            else
                return { argument_type::ERROR, "Windows standard requires non-positional arguments with a preceeding slash ('\\')." };
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
            return "Windows style options (equal signed form)";
        }

        std::string short_prec() const override {
            return "/";
        }

        std::string long_prec() const override {
            return "/";
        }

        static windows_equal_argument_convention instance;
    private:
        windows_equal_argument_convention() = default;
    };
}

namespace argument_parser::conventions {
    static inline const implementations::windows_argument_convention windows_argument_convention = implementations::windows_argument_convention::instance;
	static inline const implementations::windows_equal_argument_convention windows_equal_argument_convention = implementations::windows_equal_argument_convention::instance;
}

#endif // WINDOWS_ARGUMENT_CONVENTION_HPP