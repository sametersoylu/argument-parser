#pragma once

#ifdef __linux__
#ifndef LINUX_PARSER_HPP
#include <fstream>
#include <string>

#include <argument_parser.hpp>

namespace argument_parser {
    class linux_parser : public base_parser {
        public: 
        linux_parser() {
            std::ifstream command_line_file{"/proc/self/cmdline"};
            std::getline(command_line_file, program_name, '\0');
            for(std::string line; std::getline(command_line_file, line, '\0');) {
                parsed_arguments.emplace_back(line); 
            }
        }
    }; 
}

#endif 
#endif