#pragma once
#ifdef __APPLE__
#ifndef MACOS_PARSER_HPP
#define MACOS_PARSER_HPP

#include <argument_parser.hpp>
#include <crt_externs.h>
#include <parser_v2.hpp>
#include <string>

namespace argument_parser {
    class macos_parser : public base_parser {
        public: 
        macos_parser() {
            const int argc = *_NSGetArgc();
            if (char **argv = *_NSGetArgv(); argc > 0 && argv != nullptr && argv[0] != nullptr) {
                program_name = (argv[0]);
                for (int i = 1; i < argc; ++i) {
                    if (argv[i] != nullptr) parsed_arguments.emplace_back(argv[i]);
                }
            }
        }
    }; 

    namespace v2 {
        class macos_parser : public v2::base_parser {
            public: 
            macos_parser() {
                const int argc = *_NSGetArgc();
                if (char **argv = *_NSGetArgv(); argc > 0 && argv != nullptr && argv[0] != nullptr) {
                    set_program_name(argv[0]);
                    for (int i = 1; i < argc; ++i) {
                        if (argv[i] != nullptr) ref_parsed_args().emplace_back(argv[i]);
                    }
                }
            }
        };    
    }
}

#endif
#endif