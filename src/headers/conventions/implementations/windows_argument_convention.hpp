#pragma once
#include "base_convention.hpp"

#ifndef WINDOWS_ARGUMENT_CONVENTION_HPP
#define WINDOWS_ARGUMENT_CONVENTION_HPP

#ifndef ALLOW_DASH_FOR_WINDOWS
#define ALLOW_DASH_FOR_WINDOWS 1
#endif

namespace argument_parser::conventions::implementations {
	class windows_argument_convention : public base_convention {
	public:
		explicit windows_argument_convention(bool accept_dash = true);
		parsed_argument get_argument(std::string const &raw) const override;
		std::string extract_value(std::string const & /*raw*/) const override;
		bool requires_next_token() const override;
		std::string name() const override;
		std::string short_prec() const override;
		std::string long_prec() const override;
		std::string make_help_text(std::string const &short_arg, std::string const &long_arg,
								   bool requires_value) const override;
		std::vector<convention_features> get_features() const override;

		static windows_argument_convention instance;

	private:
		bool accept_dash_;
	};

	class windows_kv_argument_convention : public base_convention {
	public:
		explicit windows_kv_argument_convention(bool accept_dash = true);
		parsed_argument get_argument(std::string const &raw) const override;
		std::string extract_value(std::string const &raw) const override;
		bool requires_next_token() const override;
		std::string name() const override;
		std::string short_prec() const override;
		std::string long_prec() const override;
		std::string make_help_text(std::string const &short_arg, std::string const &long_arg,
								   bool requires_value) const override;
		std::vector<convention_features> get_features() const override;

		static windows_kv_argument_convention instance;

	private:
		bool accept_dash_;
	};

	inline windows_argument_convention windows_argument_convention::instance =
		windows_argument_convention(bool(ALLOW_DASH_FOR_WINDOWS));
	inline windows_kv_argument_convention windows_kv_argument_convention::instance =
		windows_kv_argument_convention(bool(ALLOW_DASH_FOR_WINDOWS));
} // namespace argument_parser::conventions::implementations

namespace argument_parser::conventions {
	static inline const implementations::windows_argument_convention windows_argument_convention =
		implementations::windows_argument_convention::instance;
	static inline const implementations::windows_kv_argument_convention windows_equal_argument_convention =
		implementations::windows_kv_argument_convention::instance;
} // namespace argument_parser::conventions

#endif // WINDOWS_ARGUMENT_CONVENTION_HPP