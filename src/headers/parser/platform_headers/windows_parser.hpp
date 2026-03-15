#pragma once
#ifdef _WIN32
#include <argument_parser.hpp>

namespace argument_parser {
	class windows_parser : public base_parser {
	public:
		windows_parser();
	};
} // namespace argument_parser
#endif