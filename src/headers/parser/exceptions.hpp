#pragma once

#ifndef ARGUMENT_PARSER_EXCEPTIONS_HPP
#define ARGUMENT_PARSER_EXCEPTIONS_HPP

#include <stdexcept>

namespace argument_parser {
	namespace parser {
		class unknown_argument_exception : public std::runtime_error {
		public:
			unknown_argument_exception(const std::string &message) : std::runtime_error(message) {}
		};

	} // namespace parser
} // namespace argument_parser

#endif // ARGUMENT_PARSER_EXCEPTIONS_HPP
