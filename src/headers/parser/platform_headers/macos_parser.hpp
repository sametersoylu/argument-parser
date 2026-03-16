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
		macos_parser();
	};

	namespace v2 {
		class macos_parser : public v2::base_parser {
		public:
			macos_parser();
		};
	} // namespace v2
} // namespace argument_parser

#endif
#endif