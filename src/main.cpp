#include "../include/argument_parser.hpp"
#include "../include/linux_parser.hpp"
#include "../include/gnu_argument_convention.hpp"
#include <initializer_list>
#include <iostream>

using namespace argument_parser::conventions;

int main() {
    auto parametered_action = argument_parser::helpers::make_parametered_action_ptr<std::string>([](std::string const& test) {
        std::cout << test << std::endl; 
    });

    auto parser = argument_parser::linux_parser{}; 
    parser.add_argument("e", "echo", "echoes given variable", *parametered_action, false); 
    parser.add_argument("re", "required_echo", "required echo", *parametered_action, true); 

    std::initializer_list<argument_parser::conventions::convention const* const> conventions = {
        &gnu_argument_convention,
        &gnu_equal_argument_convention 
    }; 

    parser.add_argument("h", "help", "Displays this help text.", argument_parser::helpers::make_non_parametered_action([&parser, conventions]{
        parser.display_help(conventions); 
    }), false);

    parser.handle_arguments(conventions);

    

    return 0; 
}