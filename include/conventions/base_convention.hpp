#pragma once
#include <string>
#include <utility>

#ifndef BASE_CONVENTION_HPP
#define BASE_CONVENTION_HPP

namespace argument_parser::conventions {
    enum class argument_type {
        SHORT,
        LONG,
		POSITIONAL,
        INTERCHANGABLE,
        ERROR
    };

    using parsed_argument = std::pair<argument_type, std::string>; 

    class base_convention {
        public: 
        virtual std::string extract_value(std::string const&) const = 0; 
        virtual parsed_argument get_argument(std::string const&) const = 0;
        virtual bool requires_next_token() const = 0;
        virtual std::string name() const = 0; 
        virtual std::string short_prec() const = 0;
        virtual std::string long_prec() const = 0;  
        protected:
        base_convention() = default;
        ~base_convention() = default;
    };
    using convention = base_convention; 
}

#endif