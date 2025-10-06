#include <argparse>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <regex>

using namespace argument_parser::conventions;

struct Point {
    int x, y;
};

template<>
struct argument_parser::parsing_traits::parser_trait<Point> {
    static Point parse(const std::string& input) {
        auto comma_pos = input.find(',');
        if (comma_pos == std::string::npos) {
            throw std::runtime_error("Invalid Point format. Expected 'x,y'.");
        }
        int x = std::stoi(input.substr(0, comma_pos));
        int y = std::stoi(input.substr(comma_pos + 1));
        return {x, y};
    }
};

template<>
struct argument_parser::parsing_traits::parser_trait<std::regex> {
    static std::regex parse(const std::string& input) {
        return std::regex(input);
    }
};

const std::initializer_list<argument_parser::conventions::convention const* const> conventions = {
    &gnu_argument_convention,
    &gnu_equal_argument_convention 
}; 

const auto echo = argument_parser::helpers::make_parametered_action<std::string>([](std::string const& text) {
    std::cout << text << std::endl; 
});


const auto echo_point = argument_parser::helpers::make_parametered_action<Point>([](Point const& point) {
    std::cout << "Point(" << point.x << ", " << point.y << ")" << std::endl; 
});

const auto cat = argument_parser::helpers::make_parametered_action<std::string>([](std::string const& file_name) {
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

auto grep(argument_parser::base_parser& parser, std::string const& filename, std::regex const& pattern) {
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

int main() {
    auto parser = argument_parser::parser{};

    parser.add_argument("e", "echo", "echoes given variable", echo, false);
    parser.add_argument("ep", "echo-point", "echoes given point", echo_point, false);
    parser.add_argument<std::string>("f", "file", "File to grep, required only if using grep",  false);
    parser.add_argument<std::regex>("g", "grep", "Grep pattern, required only if using grep", false);
    parser.add_argument("c", "cat", "Prints the content of the file", cat, false);
    parser.add_argument("h", "help", "Displays this help text.", argument_parser::helpers::make_non_parametered_action([&parser]{
        parser.display_help(conventions); 
    }), false);

    parser.add_argument<Point>("p", "point", "Test point", false);
    parser.handle_arguments(conventions);
    
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
    
    return 0; 
}