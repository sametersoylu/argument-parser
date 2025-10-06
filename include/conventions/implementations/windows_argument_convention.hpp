#pragma once
#include "base_convention.hpp"
#include <stdexcept>

#ifndef WINDOWS_ARGUMENT_CONVENTION_HPP
#define WINDOWS_ARGUMENT_CONVENTION_HPP

#ifndef ALLOW_DASH_FOR_WINDOWS
#define ALLOW_DASH_FOR_WINDOWS 1
#endif

namespace argument_parser::conventions::implementations {
    class windows_argument_convention : public base_convention {
    public:
        explicit windows_argument_convention(bool accept_dash = true)
            : accept_dash_(accept_dash) {
        }

        parsed_argument get_argument(std::string const& raw) const override {
            if (raw.empty()) {
                return { argument_type::ERROR, "Empty argument token." };
            }
            const char c0 = raw[0];
            const bool ok_prefix = (c0 == '/') || (accept_dash_ && c0 == '-');
            if (!ok_prefix) {
                return { argument_type::ERROR,
                    accept_dash_
                        ? "Windows-style expects options to start with '/' (or '-' in compat mode)."
                        : "Windows-style expects options to start with '/'." };
            }

            if (raw.find_first_of("=:") != std::string::npos) {
                return { argument_type::ERROR,
                         "Inline values are not allowed in this convention; provide the value in the next token." };
            }

            std::string name = helpers::to_lower(raw.substr(1));
            if (name.empty()) {
                return { argument_type::ERROR, "Option name cannot be empty after '/'." };
            }

            return { argument_type::INTERCHANGABLE, std::move(name) };
        }

        std::string extract_value(std::string const& /*raw*/) const override {
            throw std::runtime_error("No inline value; value must be provided in the next token.");
        }

        bool requires_next_token() const override { return true; }

        std::string name() const override { return "Windows style options (next-token values)"; }
        std::string short_prec() const override { return accept_dash_ ? "-" : "/"; }
        std::string long_prec()  const override { return "/"; }

        static windows_argument_convention instance; 
    private:
        bool accept_dash_;
    };

    class windows_kv_argument_convention : public base_convention {
    public:
        explicit windows_kv_argument_convention(bool accept_dash = true)
            : accept_dash_(accept_dash) {
        }

        parsed_argument get_argument(std::string const& raw) const override {
            if (raw.empty()) {
                return { argument_type::ERROR, "Empty argument token." };
            }
            const char c0 = raw[0];
            const bool ok_prefix = (c0 == '/') || (accept_dash_ && c0 == '-');
            if (!ok_prefix) {
                return { argument_type::ERROR,
                    accept_dash_
                        ? "Windows-style expects options to start with '/' (or '-' in compat mode)."
                        : "Windows-style expects options to start with '/'." };
            }

            const std::size_t sep = raw.find_first_of("=:");
            if (sep == std::string::npos) {
                return { argument_type::ERROR,
                         "Expected an inline value using '=' or ':' (e.g., /opt=value or /opt:value)." };
            }
            if (sep == 1) { 
                return { argument_type::ERROR, "Option name cannot be empty before '=' or ':'." };
            }

            std::string name = helpers::to_lower(raw.substr(1, sep - 1));
            return { argument_type::INTERCHANGABLE, std::move(name) };
        }

        std::string extract_value(std::string const& raw) const override {
            const std::size_t sep = raw.find_first_of("=:");
            if (sep == std::string::npos || sep + 1 >= raw.size())
                throw std::runtime_error("Expected a value after '=' or ':'.");
            return raw.substr(sep + 1);
        }

        bool requires_next_token() const override { return false; }

        std::string name() const override { return "Windows style options (inline values via '=' or ':')"; }
        std::string short_prec() const override { return accept_dash_ ? "-" : "/"; }
        std::string long_prec()  const override { return "/"; }

        static windows_kv_argument_convention instance;
    private:
        bool accept_dash_;
    };


	inline windows_argument_convention windows_argument_convention::instance = windows_argument_convention(bool(ALLOW_DASH_FOR_WINDOWS));
	inline windows_kv_argument_convention windows_kv_argument_convention::instance = windows_kv_argument_convention(bool(ALLOW_DASH_FOR_WINDOWS));
}

namespace argument_parser::conventions {
    static inline const implementations::windows_argument_convention windows_argument_convention = implementations::windows_argument_convention::instance;
	static inline const implementations::windows_kv_argument_convention windows_equal_argument_convention = implementations::windows_kv_argument_convention::instance;
}

#endif // WINDOWS_ARGUMENT_CONVENTION_HPP