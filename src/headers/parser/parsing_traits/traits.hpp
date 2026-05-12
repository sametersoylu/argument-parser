// ReSharper disable CppFunctionIsNotImplemented
#pragma once
#ifndef PARSING_TRAITS_HPP
#define PARSING_TRAITS_HPP

#include <string>

namespace argument_parser::parsing_traits {
	using hint_type = const char *;

	template <typename T_> struct parser_trait {
		using type = T_;
		static T_ parse(const std::string &input);
		static bool validate(T_ const &);

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

	constexpr hint_type comma = ",";
	template <const hint_type *PtrAddr> struct hint_provider {
		static constexpr hint_type value = *PtrAddr;
	};

	template <typename... Providers> struct joiner {
		static constexpr auto get_combined() {
			constexpr size_t total_len = (std::string_view{Providers::value}.length() + ... + 0);

			std::array<char, total_len + 1> arr{};
			// ReSharper disable once CppDFAUnreadVariable
			size_t offset = 0;
			auto append = [&](const hint_type s) {
				const std::string_view sv{s};
				for (char c : sv)
					arr[offset++] = c;
				return 0;
			};

			(append(Providers::value), ...);

			arr[total_len] = '\0';
			return arr;
		}

		static constexpr auto storage = get_combined();
		static constexpr hint_type value = storage.data();
	};

	template <typename... Providers> constexpr hint_type concat = joiner<Providers...>::value;

} // namespace argument_parser::parsing_traits

#endif
