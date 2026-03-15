#pragma once

#ifdef __linux__
#ifndef LINUX_PARSER_HPP
#define LINUX_PARSER_HPP

#include <argument_parser.hpp>

namespace argument_parser {
	class linux_parser : public base_parser {
	public:
		linux_parser();
	};
} // namespace argument_parser

#endif
#endif