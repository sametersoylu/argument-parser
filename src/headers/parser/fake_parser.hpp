#pragma once

#include "parser_v2.hpp"
#ifndef FAKE_PARSER_HPP
#define FAKE_PARSER_HPP

#include <argument_parser.hpp>
#include <initializer_list>
#include <string>

namespace argument_parser {
	class fake_parser : public base_parser {
	public:
		fake_parser() = default;
		fake_parser(std::string program_name, std::vector<std::string> const &arguments);
		fake_parser(std::string const &program_name, std::vector<std::string> &&arguments);
		fake_parser(std::string const &program_name, std::initializer_list<std::string> const &arguments);

		void set_program_name(std::string const &program_name);
		void set_parsed_arguments(std::vector<std::string> const &parsed_arguments);
	};

	namespace v2 {
		class fake_parser : public argument_parser::v2::base_parser {
		public:
			fake_parser() = default;
			fake_parser(std::string program_name, std::vector<std::string> const &arguments);
			fake_parser(std::string program_name, std::vector<std::string> &&arguments);
			fake_parser(std::string program_name, std::initializer_list<std::string> const &arguments);

			void set_program_name(std::string const &program_name);
			void set_parsed_arguments(std::vector<std::string> const &parsed_arguments);
		};
	} // namespace v2
} // namespace argument_parser

#endif
