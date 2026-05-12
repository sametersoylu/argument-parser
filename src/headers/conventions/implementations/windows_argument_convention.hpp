#pragma once
#include "base_convention.hpp"

#ifndef WINDOWS_ARGUMENT_CONVENTION_HPP
#define WINDOWS_ARGUMENT_CONVENTION_HPP

#ifndef ALLOW_DASH_FOR_WINDOWS
#define ALLOW_DASH_FOR_WINDOWS true
#endif

namespace argument_parser::conventions::implementations {
	class windows_argument_convention : public base_convention {
	public:
		virtual ~windows_argument_convention() = default;
		explicit windows_argument_convention(bool accept_dash = true);
		[[nodiscard]] parsed_argument get_argument(std::string const &raw) const override;
		[[nodiscard]] std::string extract_value(std::string const & /*raw*/) const override;
		[[nodiscard]] bool requires_next_token() const override;
		[[nodiscard]] std::string name() const override;
		[[nodiscard]] std::string short_prec() const override;
		[[nodiscard]] std::string long_prec() const override;
		[[nodiscard]] std::pair<std::string, std::string> make_help_text(std::string const &short_arg, std::string const &long_arg,
														   bool requires_value) const override;
		[[nodiscard]] std::vector<convention_features> get_features() const override;

		static windows_argument_convention instance;

	private:
		bool accept_dash_;
	};

	class windows_kv_argument_convention : public base_convention {
	public:
		virtual ~windows_kv_argument_convention() = default;
		explicit windows_kv_argument_convention(bool accept_dash = true);
		[[nodiscard]] parsed_argument get_argument(std::string const &raw) const override;
		[[nodiscard]] std::string extract_value(std::string const &raw) const override;
		[[nodiscard]] bool requires_next_token() const override;
		[[nodiscard]] std::string name() const override;
		[[nodiscard]] std::string short_prec() const override;
		[[nodiscard]] std::string long_prec() const override;
		[[nodiscard]] std::pair<std::string, std::string> make_help_text(std::string const &short_arg, std::string const &long_arg,
														   bool requires_value) const override;
		[[nodiscard]] std::vector<convention_features> get_features() const override;

		static windows_kv_argument_convention instance;

	private:
		bool accept_dash_;
	};

	inline windows_argument_convention windows_argument_convention::instance =
		windows_argument_convention(ALLOW_DASH_FOR_WINDOWS);
	inline windows_kv_argument_convention windows_kv_argument_convention::instance =
		windows_kv_argument_convention(ALLOW_DASH_FOR_WINDOWS);
} // namespace argument_parser::conventions::implementations

namespace argument_parser::conventions {
	static inline const implementations::windows_argument_convention windows_argument_convention =
		implementations::windows_argument_convention::instance;
	static inline const implementations::windows_kv_argument_convention windows_equal_argument_convention =
		implementations::windows_kv_argument_convention::instance;
} // namespace argument_parser::conventions

#endif // WINDOWS_ARGUMENT_CONVENTION_HPP
