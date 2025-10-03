#pragma once

#ifndef FAKE_PARSER_HPP
#define FAKE_PARSER_HPP

#include <argument_parser.hpp>
#include <initializer_list>
#include <string>

namespace argument_parser {
    class fake_parser : public base_parser {
        public: 
        fake_parser(std::string const& program_name, std::vector<std::string> const& arguments) {
            this->program_name = program_name;
            parsed_arguments = arguments;
        }

        fake_parser(std::string const& program_name, std::vector<std::string>&& arguments) {
            this->program_name = program_name;
            parsed_arguments = std::move(arguments);
        }

        fake_parser(std::string const& program_name, std::initializer_list<std::string> const& arguments) : 
            fake_parser(program_name, std::vector<std::string>(arguments)) {}
    }; 
}

#endif