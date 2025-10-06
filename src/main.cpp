#define ALLOW_DASH_FOR_WINDOWS 0

#include <argparse>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>


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

template<>
struct argument_parser::parsing_traits::parser_trait<std::vector<int>> {
    static std::vector<int> parse(const std::string& input) {
        std::vector<int> result;
        std::stringstream ss(input);
        std::string item;
        while (std::getline(ss, item, ',')) {
            result.push_back(std::stoi(item));
        }
        return result;
    }
};


template<>
struct argument_parser::parsing_traits::parser_trait<std::vector<std::string>> {
    static std::vector<std::string> parse(const std::string& input) {
        std::vector<std::string> result;
        auto copyInput = input;
        if (input.starts_with("[")) {
            copyInput = input.substr(1, input.size());
            if (copyInput.ends_with("]")) {
                copyInput = copyInput.substr(0, copyInput.size() - 1);
            } else {
                throw std::runtime_error("Invalid vector<string> format. Expected closing ']'.");
            }
        }
        std::stringstream ss(copyInput);
        std::string item;
        while (std::getline(ss, item, ',')) {
            result.push_back(item);
        }
        return result;
    }
};

const std::initializer_list<argument_parser::conventions::convention const* const> conventions = {
    &argument_parser::conventions::gnu_argument_convention,
    &argument_parser::conventions::gnu_equal_argument_convention,
	&argument_parser::conventions::windows_argument_convention,
	&argument_parser::conventions::windows_equal_argument_convention
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

auto grep(argument_parser::base_parser const& parser, std::string const& filename, std::regex const& pattern) {
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

void run_grep(argument_parser::base_parser const& parser) {
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

    parser.add_argument<std::vector<int>>("t", "test", "Test vector<int>", false);
    parser.add_argument<std::vector<std::string>>("ts", "test-strings", "Test vector<string>", false);
    parser.on_complete(::run_grep); 
    try {
        parser.handle_arguments(conventions);
    } catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl; 
        parser.display_help(conventions); 
        return -1; 
	}
    
   
    auto test = parser.get_optional<std::vector<int>>("test");
    if (test) {
        for (auto const& item : test.value()) {
            std::cout << item << std::endl;
        }
    }

    auto test_strings = parser.get_optional<std::vector<std::string>>("test-strings");
    if (test_strings) {
        for (auto const& item : test_strings.value()) {
            std::cout << item << std::endl;
        }
    }

    return 0; 
}