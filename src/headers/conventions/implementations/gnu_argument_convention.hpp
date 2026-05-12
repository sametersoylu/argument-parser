#pragma once
#include "base_convention.hpp"

#ifndef GNU_ARGUMENT_CONVENTION_HPP
#define GNU_ARGUMENT_CONVENTION_HPP

namespace argument_parser::conventions::implementations {

	class gnu_argument_convention : public base_convention {
	public:
		virtual ~gnu_argument_convention() = default;
		[[nodiscard]] parsed_argument get_argument(std::string const &raw) const override;
		[[nodiscard]] std::string extract_value(std::string const & /*raw*/) const override;
		[[nodiscard]] bool requires_next_token() const override;
		[[nodiscard]] std::string name() const override;
		[[nodiscard]] std::string short_prec() const override;
		[[nodiscard]] std::string long_prec() const override;
		[[nodiscard]] std::pair<std::string, std::string> make_help_text(std::string const &short_arg, std::string const &long_arg,
														   bool requires_value) const override;
		[[nodiscard]] std::vector<convention_features> get_features() const override;

		static gnu_argument_convention instance;

	private:
		gnu_argument_convention() = default;
	};

	class gnu_equal_argument_convention : public base_convention {
	public:
		virtual ~gnu_equal_argument_convention() = default;
		[[nodiscard]] parsed_argument get_argument(std::string const &raw) const override;
		[[nodiscard]] std::string extract_value(std::string const &raw) const override;
		[[nodiscard]] bool requires_next_token() const override;
		[[nodiscard]] std::string name() const override;
		[[nodiscard]] std::string short_prec() const override;
		[[nodiscard]] std::string long_prec() const override;
		[[nodiscard]] std::pair<std::string, std::string> make_help_text(std::string const &short_arg, std::string const &long_arg,
														   bool requires_value) const override;
		[[nodiscard]] std::vector<convention_features> get_features() const override;

		static gnu_equal_argument_convention instance;

	private:
		gnu_equal_argument_convention() = default;
	};

	inline gnu_argument_convention gnu_argument_convention::instance{};
	inline gnu_equal_argument_convention gnu_equal_argument_convention::instance{};
} // namespace argument_parser::conventions::implementations

namespace argument_parser::conventions {
	static const implementations::gnu_argument_convention gnu_argument_convention =
		implementations::gnu_argument_convention::instance;
	static const implementations::gnu_equal_argument_convention gnu_equal_argument_convention =
		implementations::gnu_equal_argument_convention::instance;
} // namespace argument_parser::conventions

#endif
