#pragma once
#ifndef PARSING_TRAITS_HPP
#define PARSING_TRAITS_HPP

#include <string>

namespace argument_parser::parsing_traits {
	template <typename T_> struct parser_trait {
		using type = T_;
		static T_ parse(const std::string &input);
	};

	template <> struct parser_trait<std::string> {
		static std::string parse(const std::string &input);
	};

	template <> struct parser_trait<bool> {
		static bool parse(const std::string &input);
	};

	template <> struct parser_trait<int> {
		static int parse(const std::string &input);
	};

	template <> struct parser_trait<float> {
		static float parse(const std::string &input);
	};

	template <> struct parser_trait<double> {
		static double parse(const std::string &input);
	};
} // namespace argument_parser::parsing_traits

#endif