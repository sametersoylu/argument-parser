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

	namespace v2 {
		linux_parser::linux_parser() {
			std::ifstream command_line_file{"/proc/self/cmdline"};
			std::string program_name;
			std::getline(command_line_file, program_name, '\0');
			set_program_name(program_name);

			for (std::string line; std::getline(command_line_file, line, '\0');) {
				parsed_arguments.emplace_back(line);
			}

			add_argument({{flags::ShortArgument, "h"},
						  {flags::LongArgument, "help"},
						  {flags::Action, helpers::make_non_parametered_action([this]() {
							   this->display_help(this->current_conventions());
							   std::exit(0);
						   })},
						  {flags::HelpText, "Prints this help text."}});
		}
	} // namespace v2
} // namespace argument_parser

#endif
