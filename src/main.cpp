#include <string>
#define ALLOW_DASH_FOR_WINDOWS 0

#include <argparse>
#include <fstream>
#include <iostream>
#include <parser_v2.hpp>
#include <regex>
#include <sstream>
#include <vector>

struct Point {
	int x, y;
};

template <> struct argument_parser::parsing_traits::parser_trait<Point> {
	static Point parse(const std::string &input) {
		auto comma_pos = input.find(',');
		if (comma_pos == std::string::npos) {
			throw std::runtime_error("Invalid Point format. Expected 'x,y'.");
		}
		int x = std::stoi(input.substr(0, comma_pos));
		int y = std::stoi(input.substr(comma_pos + 1));
		return {x, y};
	}
};

template <> struct argument_parser::parsing_traits::parser_trait<std::regex> {
	static std::regex parse(const std::string &input) {
		return std::regex(input);
	}
};

template <> struct argument_parser::parsing_traits::parser_trait<std::vector<int>> {
	static std::vector<int> parse(const std::string &input) {
		std::vector<int> result;
		std::stringstream ss{input};
		std::string item;
		while (std::getline(ss, item, ',')) {
			result.push_back(std::stoi(item));
		}
		return result;
	}
};

template <> struct argument_parser::parsing_traits::parser_trait<std::vector<std::string>> {
	static std::vector<std::string> parse(const std::string &input) {
		std::vector<std::string> result;
		std::stringstream ss{input};
		std::string item;
		while (std::getline(ss, item, ',')) {
			result.push_back(item);
		}
		return result;
	}
};

const std::initializer_list<argument_parser::conventions::convention const *const> conventions = {
	&argument_parser::conventions::gnu_argument_convention,
	&argument_parser::conventions::gnu_equal_argument_convention,
	&argument_parser::conventions::windows_argument_convention,
	&argument_parser::conventions::windows_equal_argument_convention};

const auto echo = argument_parser::helpers::make_parametered_action<std::string>(
	[](std::string const &text) { std::cout << text << std::endl; });

const auto echo_point = argument_parser::helpers::make_parametered_action<Point>(
	[](Point const &point) { std::cout << "Point(" << point.x << ", " << point.y << ")" << std::endl; });

const auto cat = argument_parser::helpers::make_parametered_action<std::string>([](std::string const &file_name) {
	std::ifstream file(file_name);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file");
	}
	std::string line;
	while (std::getline(file, line)) {
		std::cout << line << std::endl;
	}

	file.close();
});

auto grep(argument_parser::base_parser const &parser, std::string const &filename, std::regex const &pattern) {
	if (filename.empty()) {
		std::cerr << "Missing filename" << std::endl;
		parser.display_help(conventions);
		exit(-1);
	}
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Could not open file: \"" << filename << '"' << std::endl;
		exit(-1);
	}

	for (std::string line; std::getline(file, line);) {
		if (std::regex_search(line, pattern)) {
			std::cout << line << std::endl;
		}
	}

	file.close();
}

void run_grep(argument_parser::base_parser const &parser) {
	auto filename = parser.get_optional<std::string>("file");
	auto pattern = parser.get_optional<std::regex>("grep");

	if (filename && pattern) {
		grep(parser, filename.value(), pattern.value());
	} else if (filename) {
		std::cerr << "Missing grep pattern" << std::endl;
		parser.display_help(conventions);
		exit(-1);
	} else if (pattern) {
		std::cerr << "Missing filename" << std::endl;
		parser.display_help(conventions);
		exit(-1);
	}
}

void run_store_point(argument_parser::base_parser const &parser) {
	auto point = parser.get_optional<Point>("store-point");
	if (point) {
		std::cout << "Point(" << point->x << ", " << point->y << ")" << std::endl;
	}
}

int v2Examples() {
	using namespace argument_parser::v2::flags;
	argument_parser::v2::parser parser;

	parser.add_argument<std::string>(
		{{ShortArgument, "e"}, {LongArgument, "echo"}, {Action, echo}, {HelpText, "echoes given variable"}});

	parser.add_argument<Point>(
		{{ShortArgument, "ep"}, {LongArgument, "echo-point"}, {Action, echo_point}, {HelpText, "echoes given point"}});

	parser.add_argument<std::string>({
		// stores string for f/file flag
		{ShortArgument, "f"},
		{LongArgument, "file"},
		{HelpText, "File to grep, required only if using grep"},
		// if no action, falls to store operation with given type.
	});

	parser.add_argument<std::regex>({
		// stores string for g/grep flag
		{ShortArgument, "g"},
		{LongArgument, "grep"},
		{HelpText, "Grep pattern, required only if using file"},
		// same as 'file' flag
	});

	parser.add_argument<std::string>(
		{{ShortArgument, "c"}, {LongArgument, "cat"}, {Action, cat}, {HelpText, "Prints the content of the file"}});

	parser.add_argument<Point>({
		// { ShortArgument, "sp" }, // now if ShortArgument or LongArgument is missing, it will use it for the other.
		{LongArgument, "store-point"},
		{Required, true} // makes this flag required
	});

	parser.on_complete(::run_grep);
	parser.on_complete(::run_store_point);

	parser.handle_arguments(conventions);
	return 0;
}

int main() {
	try {
		return v2Examples();
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
	}
}