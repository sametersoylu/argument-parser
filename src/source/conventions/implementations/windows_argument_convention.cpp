#include "windows_argument_convention.hpp"

namespace argument_parser::conventions::implementations {
	windows_argument_convention::windows_argument_convention(bool accept_dash) : accept_dash_(accept_dash) {}

	parsed_argument windows_argument_convention::get_argument(std::string const &raw) const {
		if (raw.empty()) {
			return {argument_type::ERROR, "Empty argument token."};
		}
		const char c0 = raw[0];
		const bool ok_prefix = (c0 == '/') || (accept_dash_ && c0 == '-');
		if (!ok_prefix) {
			return {argument_type::ERROR,
					accept_dash_ ? "Windows-style expects options to start with '/' (or '-' in compat mode)."
								 : "Windows-style expects options to start with '/'."};
		}

		if (raw.find_first_of("=:") != std::string::npos) {
			return {argument_type::ERROR,
					"Inline values are not allowed in this convention; provide the value in the next token."};
		}

		std::string name = helpers::to_lower(raw.substr(1));
		if (name.empty()) {
			return {argument_type::ERROR, "Option name cannot be empty after '/'."};
		}

		return {argument_type::INTERCHANGABLE, std::move(name)};
	}

	std::string windows_argument_convention::extract_value(std::string const & /*raw*/) const {
		throw std::runtime_error("No inline value; value must be provided in the next token.");
	}

	bool windows_argument_convention::requires_next_token() const {
		return true;
	}

	std::string windows_argument_convention::name() const {
		return "Windows style options (next-token values)";
	}

	std::string windows_argument_convention::short_prec() const {
		return accept_dash_ ? "-" : "/";
	}

	std::string windows_argument_convention::long_prec() const {
		return "/";
	}

} // namespace argument_parser::conventions::implementations

namespace argument_parser::conventions::implementations {
	windows_kv_argument_convention::windows_kv_argument_convention(bool accept_dash) : accept_dash_(accept_dash) {}

	parsed_argument windows_kv_argument_convention::get_argument(std::string const &raw) const {
		if (raw.empty()) {
			return {argument_type::ERROR, "Empty argument token."};
		}
		const char c0 = raw[0];
		const bool ok_prefix = (c0 == '/') || (accept_dash_ && c0 == '-');
		if (!ok_prefix) {
			return {argument_type::ERROR,
					accept_dash_ ? "Windows-style expects options to start with '/' (or '-' in compat mode)."
								 : "Windows-style expects options to start with '/'."};
		}

		const std::size_t sep = raw.find_first_of("=:");
		if (sep == std::string::npos) {
			return {argument_type::ERROR,
					"Expected an inline value using '=' or ':' (e.g., /opt=value or /opt:value)."};
		}
		if (sep == 1) {
			return {argument_type::ERROR, "Option name cannot be empty before '=' or ':'."};
		}

		std::string name = helpers::to_lower(raw.substr(1, sep - 1));
		return {argument_type::INTERCHANGABLE, std::move(name)};
	}

	std::string windows_kv_argument_convention::extract_value(std::string const &raw) const {
		const std::size_t sep = raw.find_first_of("=:");
		if (sep == std::string::npos || sep + 1 >= raw.size())
			throw std::runtime_error("Expected a value after '=' or ':'.");
		return raw.substr(sep + 1);
	}

	bool windows_kv_argument_convention::requires_next_token() const {
		return false;
	}

	std::string windows_kv_argument_convention::name() const {
		return "Windows-style options (inline values via '=' or ':')";
	}

	std::string windows_kv_argument_convention::short_prec() const {
		return accept_dash_ ? "-" : "/";
	}

	std::string windows_kv_argument_convention::long_prec() const {
		return "/";
	}
} // namespace argument_parser::conventions::implementations