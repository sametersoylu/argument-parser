#pragma once

#ifdef __linux__
#ifndef LINUX_PARSER_HPP
#define LINUX_PARSER_HPP

#include <argument_parser.hpp>
#include <parser_v2.hpp>

namespace argument_parser {
	class linux_parser : public base_parser {
	public:
		linux_parser();
	};

	namespace v2 {
		class linux_parser : public v2::base_parser {
		public:
			linux_parser();
			using base_parser::display_help;
		};
	} // namespace v2
} // namespace argument_parser

#endif
#endif