#pragma once
#ifdef _WIN32
#include <argument_parser.hpp>
#include <parser_v2.hpp>

namespace argument_parser {
	class windows_parser : public base_parser {
	public:
		windows_parser();
	};

	namespace v2 {
		class windows_parser : public v2::base_parser {
		public:
			windows_parser();
			using base_parser::display_help;
		};
	} // namespace v2
} // namespace argument_parser
#endif