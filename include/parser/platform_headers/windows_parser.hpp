#pragma once
#ifdef _WIN32
#ifndef WINDOWS_PARSER_HPP

#include <argument_parser.hpp>
#include <fstream>
#include <string>

namespace argument_parser {
    class windows_parser : public base_parser {
        public: 
        windows_parser() {
        }
    };

    using parser = windows_parser; 
}

#endif 
#endif