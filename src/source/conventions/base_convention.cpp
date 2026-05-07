#include "base_convention.hpp"
#include <algorithm>

namespace argument_parser::conventions::helpers {
	std::string to_lower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		return s;
	}

	std::string to_upper(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
		return s;
	}
} // namespace argument_parser::conventions::helpers
