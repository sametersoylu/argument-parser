#include "traits.hpp"
#include <stdexcept>

namespace argument_parser::parsing_traits {
	std::string parser_trait<std::string>::parse(const std::string &input) {
		return input;
	}

	bool parser_trait<bool>::parse(const std::string &input) {
		if (input == "t" || input == "true" || input == "1")
			return true;
		if (input == "f" || input == "false" || input == "0")
			return false;
		throw std::runtime_error("Invalid boolean value: " + input);
	}

	int parser_trait<int>::parse(const std::string &input) {
		return std::stoi(input);
	}

	float parser_trait<float>::parse(const std::string &input) {
		return std::stof(input);
	}

	double parser_trait<double>::parse(const std::string &input) {
		return std::stod(input);
	}
} // namespace argument_parser::parsing_traits
