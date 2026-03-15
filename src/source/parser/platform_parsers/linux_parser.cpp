#ifdef __linux__

#include "linux_parser.hpp"

#include <fstream>
#include <string>

namespace argument_parser {
	linux_parser::linux_parser() {
		std::ifstream command_line_file{"/proc/self/cmdline"};
		std::getline(command_line_file, program_name, '\0');
		for (std::string line; std::getline(command_line_file, line, '\0');) {
			parsed_arguments.emplace_back(line);
		}
	}
} // namespace argument_parser

#endif
