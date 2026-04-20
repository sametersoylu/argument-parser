#pragma once
#ifndef PARSING_TRAITS_HPP
#define PARSING_TRAITS_HPP

#include <string>

namespace argument_parser::parsing_traits {
	using hint_type = const char *;

	template <typename T_> struct parser_trait {
		using type = T_;
		static T_ parse(const std::string &input);
		static bool validate(T_ const&);

		static constexpr hint_type format_hint = "value";
		static constexpr hint_type purpose_hint = "value";
	};

	template <> struct parser_trait<std::string> {
		static std::string parse(const std::string &input);

		static constexpr hint_type format_hint = "string";
		static constexpr hint_type purpose_hint = "string value";
	};

	template <> struct parser_trait<bool> {
		static bool parse(const std::string &input);

		static constexpr hint_type format_hint = "true/false";
		static constexpr hint_type purpose_hint = "boolean value";
	};

	template <> struct parser_trait<int> {
		static int parse(const std::string &input);

		static constexpr hint_type format_hint = "123";
		static constexpr hint_type purpose_hint = "integer value";
	};

	template <> struct parser_trait<float> {
		static float parse(const std::string &input);

		static constexpr hint_type format_hint = "3.14";
		static constexpr hint_type purpose_hint = "floating point number";
	};

	template <> struct parser_trait<double> {
		static double parse(const std::string &input);

		static constexpr hint_type format_hint = "3.14";
		static constexpr hint_type purpose_hint = "double precision floating point number";
	};
} // namespace argument_parser::parsing_traits

#endif
