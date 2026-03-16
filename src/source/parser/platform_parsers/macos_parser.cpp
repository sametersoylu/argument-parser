#ifdef __APPLE__

#include "macos_parser.hpp"

namespace argument_parser {
	macos_parser::macos_parser() {
		const int argc = *_NSGetArgc();
		if (char **argv = *_NSGetArgv(); argc > 0 && argv != nullptr && argv[0] != nullptr) {
			program_name = (argv[0]);
			for (int i = 1; i < argc; ++i) {
				if (argv[i] != nullptr)
					parsed_arguments.emplace_back(argv[i]);
			}
		}
	}

	namespace v2 {
		macos_parser::macos_parser() {
			const int argc = *_NSGetArgc();
			if (char **argv = *_NSGetArgv(); argc > 0 && argv != nullptr && argv[0] != nullptr) {
				set_program_name(argv[0]);
				for (int i = 1; i < argc; ++i) {
					if (argv[i] != nullptr)
						ref_parsed_args().emplace_back(argv[i]);
				}
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