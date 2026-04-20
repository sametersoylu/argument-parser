#ifdef __APPLE__

#include "macos_parser.hpp"

#include <crt_externs.h>

#define MACOS_GETARGS_LOOP(argc_name, argv_name, before_for, for_body) \
	do { \
		const int argc_name = *_NSGetArgc(); \
		if (char **argv_name = *_NSGetArgv(); argc_name > 0 && argv_name != nullptr && argv_name[0] != nullptr) { \
			do { before_for; } while(false); \
			for (int i = 1; i < argc_name; ++i) { \
				for_body \
			} \
		} \
	} while (false)

namespace argument_parser {
	macos_parser::macos_parser() {
		MACOS_GETARGS_LOOP(argc, argv, program_name = argv[0], {
			if (argv[i] != nullptr)
				parsed_arguments.emplace_back(argv[i]);
		});
	}

	namespace v2 {
		macos_parser::macos_parser(bool should_exit) {
            MACOS_GETARGS_LOOP(argc, argv, set_program_name(argv[0]), {
    			if (argv[i] != nullptr)
					ref_parsed_args().emplace_back(argv[i]);
    		});
			prepare_help_flag(should_exit);
		}
	} // namespace v2
} // namespace argument_parser

#endif
