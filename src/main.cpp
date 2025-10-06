#include <argparse>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>

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

auto make_grep_action(argument_parser::base_parser& parser) {
    std::shared_ptr<std::string> filename = std::make_shared<std::string>(""); 
    std::shared_ptr<bool> grep_requested = std::make_shared<bool>(false);
    std::shared_ptr<bool> grep_called = std::make_shared<bool>(false);
    std::shared_ptr<std::string> pattern = std::make_shared<std::string>(); 

    auto grep_impl = [&parser] (std::string const& filename, std::string const& pattern) {
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
        std::string line;
        while (std::getline(file, line)) {
            if (line.find(pattern) != std::string::npos) {
                std::cout << line << std::endl;
            }
        }
        file.close();
    }; 

    const auto filename_action = argument_parser::helpers::make_parametered_action<std::string>([filename, grep_called, grep_requested, &grep_impl, pattern](std::string const& file_name) {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file");
        }
        *filename = file_name;
        file.close();
        if (*grep_requested && !(*grep_called)) {
            grep_impl(file_name, *pattern);
            *grep_called = true; 
        }
    });

    const auto grep = argument_parser::helpers::make_parametered_action<std::string>([filename, &parser, &grep_impl, grep_requested, grep_called, pattern](std::string const& p) {
        *grep_requested = true;
        if (!filename || filename -> empty()) {
            *grep_called = false; 
            *pattern = p; 
            return; 
        }
        grep_impl(*filename,  p);
        *grep_called = true; 
    });

    return std::pair { filename_action, grep };
}

int main() {
    auto parser = argument_parser::parser{};
    auto [file, grep] = make_grep_action(parser); 

    parser.add_argument("e", "echo", "echoes given variable", echo, false);
    parser.add_argument("ep", "echo-point", "echoes given point", echo_point, false);
    parser.add_argument("f", "file", "File to grep, required only if using grep", file, false);
    parser.add_argument("g", "grep", "Grep pattern, required only if using grep", grep, false);
    parser.add_argument("c", "cat", "Prints the content of the file", cat, false);
    parser.add_argument("h", "help", "Displays this help text.", argument_parser::helpers::make_non_parametered_action([&parser]{
        parser.display_help(conventions); 
    }), false);

    parser.add_argument<std::string>("t", "test_store", "Test store", false); 
    parser.add_argument<Point>("p", "point", "Test point", false);
    parser.handle_arguments(conventions);

    auto store = parser.get_optional<std::string>("test_store"); 
    if (store.has_value()) {
        std::cout << "Stored value: " << store.value() << std::endl;
    } else {
        std::cout << "No stored value." << std::endl;
    }

    auto point = parser.get_optional<Point>("point");
    if (point.has_value()) {
        std::cout << "Stored point: " << point.value().x << ", " << point.value().y << std::endl;
    } else {
        std::cout << "No stored point." << std::endl;
    }
    
    return 0; 
}