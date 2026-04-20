#include "macros.h"
#include "traits.hpp"
#include <exception>
#include <memory>
#include <string>

#include <argparse>
#include <fstream>
#include <iostream>
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

	static bool validate(Point const& p) {
		return p.x >= 0 && p.y >= 0;
	}

	ARGPARSE_TRAIT_FORMAT_HINT = "x,y";
	ARGPARSE_TRAIT_PURPOSE_HINT = "coordinates";
};

template <> struct argument_parser::parsing_traits::parser_trait<std::regex> {
	static std::regex parse(const std::string &input) {
		return std::regex(input);
	}
	ARGPARSE_TRAIT_FORMAT_HINT = "regex";
	ARGPARSE_TRAIT_PURPOSE_HINT = "regular expression";
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

	ARGPARSE_TRAIT_FORMAT_HINT = "string,string,string";
	ARGPARSE_TRAIT_PURPOSE_HINT = "list of strings";
};

template <typename VT> struct argument_parser::parsing_traits::parser_trait<std::vector<VT>> {
	static std::vector<VT> parse(const std::string &input) {
	    std::vector<VT> result;
	    std::stringstream ss{input};
	    std::string item;

	    while (std::getline(ss, item, ',')) {
	        result.push_back(argument_parser::parsing_traits::parser_trait<VT>::parse(item));
	    }
	    return result;
	}
	ARGPARSE_TRAIT_FORMAT_HINT = "VT,VT,VT";
	ARGPARSE_TRAIT_PURPOSE_HINT = "list of VT";
};

const std::initializer_list<argument_parser::conventions::convention const *const> conventions = {
	&argument_parser::conventions::gnu_argument_convention,
	&argument_parser::conventions::gnu_equal_argument_convention,
	// &argument_parser::conventions::windows_argument_convention,
	// &argument_parser::conventions::windows_equal_argument_convention
};

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
	argument_parser::v2::parser parser{ false };

	parser.add_argument<std::string>(
		{{ShortArgument, "e"}, {LongArgument, "echo"}, {Action, echo}, {HelpText, "echoes given variable"}});

	parser.add_argument<Point>({{ShortArgument, "ep"}, {LongArgument, "echo-point"}, {Action, echo_point}});

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
		{{ShortArgument, "c"}, {LongArgument, "cat"}, {Action, cat}, {HelpText, "Prints the content of the file"}}
	);

	parser.add_argument<Point>({
		// { ShortArgument, "sp" }, // now if ShortArgument or LongArgument is missing, it will use it for the other.
		{LongArgument, "store-point"},
		{Required, true} // makes this flag required
	});

	parser.add_argument({{ShortArgument, "v"}, {LongArgument, "verbose"}});

	parser.add_argument<std::string>({
		{Positional, "input"},
		{HelpText, "Input file to process"},
		{Required, true},
	});

	parser.add_argument<std::string>({
		{Positional, "output"},
		{HelpText, "Output file path"},
	});

	parser.add_argument<std::vector<Point>>({{LongArgument, "points"}, {HelpText, "List of points to store"}});

	parser.on_complete(::run_grep);
	parser.on_complete(::run_store_point);
	parser.on_complete([](argument_parser::base_parser const &p) {
		auto input = p.get_optional<std::string>("input");
		auto output = p.get_optional<std::string>("output");
		if (input) {
			std::cout << "Input: " << input.value() << std::endl;
		}
		if (output) {
			std::cout << "Output: " << output.value() << std::endl;
		}
	});

	parser.handle_arguments(conventions);
	return 0;
}

auto unique_copy(std::unique_ptr<std::string> ptr) {
    std::cout << *ptr << std::endl;
}

auto unique_reference(std::unique_ptr<std::string> const& ptr) {
    std::cout << *ptr << std::endl;
}

auto unique_move(std::unique_ptr<std::string>&& ptr) {
    std::cout << *ptr << std::endl;
}

template<typename T>
T return_example(std::function<T()> func) {
    if constexpr (std::is_same_v<void, T>) {
        return func();
    } else {
        return func();
    }
}

template<typename T>
void log_result(std::function<T()> func) {
    if constexpr (std::is_same_v<void, T>) {
        func();
    } else {
        std::cout << "result: " << func() << std::endl;
    }
}


int main(int argc, char **argv) {
	try {
	    return_example<void>([]{});

		return v2Examples();
	} catch (std::exception const &e) {
		std::cout << e.what() << std::endl;
	}
}
