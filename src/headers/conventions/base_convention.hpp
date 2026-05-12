#pragma once
#include <string>
#include <utility>
#include <vector>

#ifndef BASE_CONVENTION_HPP
#define BASE_CONVENTION_HPP

namespace argument_parser::conventions {
	enum class convention_features { ALLOW_SHORT_TO_LONG_FALLBACK, ALLOW_LONG_TO_SHORT_FALLBACK };
	enum class argument_type { SHORT, LONG, POSITIONAL, INTERCHANGABLE, ERROR };

	using parsed_argument = std::pair<argument_type, std::string>;

	class base_convention {
	public:
		[[nodiscard]] virtual std::string extract_value(std::string const &) const = 0;
		[[nodiscard]] virtual parsed_argument get_argument(std::string const &) const = 0;
		[[nodiscard]] virtual bool requires_next_token() const = 0;
		[[nodiscard]] virtual std::string name() const = 0;
		[[nodiscard]] virtual std::string short_prec() const = 0;
		[[nodiscard]] virtual std::string long_prec() const = 0;
		[[nodiscard]] virtual std::pair<std::string, std::string>
		make_help_text(std::string const &short_arg, std::string const &long_arg, bool requires_value) const = 0;
		[[nodiscard]] virtual std::vector<convention_features> get_features() const = 0;

	protected:
		base_convention() = default;
		~base_convention() = default;
	};
	using convention = base_convention;
} // namespace argument_parser::conventions

namespace argument_parser::conventions::helpers {
	std::string to_lower(std::string s);
	std::string to_upper(std::string s);
} // namespace argument_parser::conventions::helpers

#endif
