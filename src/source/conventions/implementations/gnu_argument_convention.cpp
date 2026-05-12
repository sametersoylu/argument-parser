#include "gnu_argument_convention.hpp"
#include "base_convention.hpp"
#include <stdexcept>

bool starts_with(std::string const &s, std::string const &prefix) {
	return s.rfind(prefix, 0) == 0;
}

namespace argument_parser::conventions::implementations {
	parsed_argument gnu_argument_convention::get_argument(std::string const &raw) const {
		if (starts_with(raw, long_prec()))
			return {argument_type::LONG, raw.substr(2)};
		if (starts_with(raw, short_prec()))
			return {argument_type::SHORT, raw.substr(1)};
		return {argument_type::ERROR, "GNU standard convention does not allow arguments without a preceding dash."};
	}

	std::string gnu_argument_convention::extract_value(std::string const & /*raw*/) const {
		throw std::runtime_error("No inline value in standard GNU convention.");
	}

	bool gnu_argument_convention::requires_next_token() const {
		return true;
	}

	std::string gnu_argument_convention::name() const {
		return "GNU-style long options";
	}

	std::string gnu_argument_convention::short_prec() const {
		return "-";
	}

	std::string gnu_argument_convention::long_prec() const {
		return "--";
	}

	std::vector<convention_features> gnu_argument_convention::get_features() const {
		return {}; // no fallback allowed
	}

	std::pair<std::string, std::string> gnu_argument_convention::make_help_text(std::string const &short_arg,
																				std::string const &long_arg,
																				bool const requires_value) const {
		std::string s_part;
		if (short_arg != "-" && !short_arg.empty()) {
			s_part += short_prec() + short_arg;
			if (requires_value) {
				s_part += " <value>";
			}
		}

		std::string l_part;
		if (long_arg != "-" && !long_arg.empty()) {
			l_part += long_prec() + long_arg;
			if (requires_value) {
				l_part += " <value>";
			}
		}

		return {s_part, l_part};
	}
} // namespace argument_parser::conventions::implementations

namespace argument_parser::conventions::implementations {
	parsed_argument gnu_equal_argument_convention::get_argument(std::string const &raw) const {
		const auto pos = raw.find('=');
		const auto arg = pos != std::string::npos ? raw.substr(0, pos) : raw;
		if (starts_with(arg, long_prec()))
			return {argument_type::LONG, arg.substr(2)};
		if (starts_with(arg, short_prec()))
			return {argument_type::SHORT, arg.substr(1)};
		return {argument_type::ERROR, "GNU standard convention does not allow arguments without a preceding dash."};
	}

	std::string gnu_equal_argument_convention::extract_value(std::string const &raw) const {
		const auto pos = raw.find('=');
		if (pos == std::string::npos || pos + 1 >= raw.size())
			throw std::runtime_error("Expected value after '='.");
		return raw.substr(pos + 1);
	}

	bool gnu_equal_argument_convention::requires_next_token() const {
		return false;
	}

	std::string gnu_equal_argument_convention::name() const {
		return "GNU-style long options (equal signed form)";
	}

	std::string gnu_equal_argument_convention::short_prec() const {
		return "-";
	}

	std::string gnu_equal_argument_convention::long_prec() const {
		return "--";
	}

	std::pair<std::string, std::string> gnu_equal_argument_convention::make_help_text(std::string const &short_arg,
																					  std::string const &long_arg,
																					  bool const requires_value) const {
		std::string s_part;
		if (short_arg != "-" && !short_arg.empty()) {
			s_part += short_prec() + short_arg;
			if (requires_value) {
				s_part += "=<value>";
			}
		}

		std::string l_part;
		if (long_arg != "-" && !long_arg.empty()) {
			l_part += long_prec() + long_arg;
			if (requires_value) {
				l_part += "=<value>";
			}
		}

		return {s_part, l_part};
	}

	std::vector<convention_features> gnu_equal_argument_convention::get_features() const {
		return {}; // no fallback allowed
	}
} // namespace argument_parser::conventions::implementations
