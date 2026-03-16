# argument-parser

A lightweight, modern, expressively typed, and highly customizable C++17 argument parser library.

## Features

- **Type-safe Argument Extraction**: Use type traits to automatically parse fundamental types and custom structures (e.g. `std::vector<int>`, `std::regex`, `Point`).
- **Support for Multiple Parsing Conventions**: Pluggable convention system out of the box, offering GNU-style (`-a`, `--arg`), GNU-equal-style (`--arg=value`), Windows-style (`/arg`), and Windows-equal-style (`/arg:value`).
- **Automated Help Text Formatting**: Call `parser.display_help(conventions)` to easily generate beautifully formatted usage instructions.
- **Cross-Platform Native Parsers**: Dedicated parsers that automatically fetch command-line arguments using OS-specific APIs (`windows_parser`, `linux_parser`, `macos_parser`), so you don't need to manually pass `argc` and `argv` on most platforms.
- **Fluid setup**: Enjoy fluid setup routines with maps and initializer lists.

### Important Note: 
V1 is deprecated and is mainly kept as a base implementation for the V2. You should use V2 for your projects. If any features are missing compared to V1, please let me know so I can introduce them!

## Requirements

- C++17 or later
- CMake 3.15 or later

## Quick Start

### 1. Create your Parser and Define Arguments

```cpp
#include <iostream>
#include <string>
#include <regex>
#include <argparse> // Provides the native parser for your compiling platform

int main() {
    using namespace argument_parser::v2::flags;
    
    // Automatically uses the platform-native parser!
    // It will fetch arguments directly from OS APIs (e.g., GetCommandLineW on Windows)
    argument_parser::v2::parser parser;

    // A flag with an action
    parser.add_argument<std::string>({
        {ShortArgument, "e"}, 
        {LongArgument, "echo"}, 
        {Action, argument_parser::helpers::make_parametered_action<std::string>(
            [](std::string const &text) { std::cout << text << std::endl; }
        )}, 
        {HelpText, "echoes given variable"}
    });

    // A flag that just stores the value to extract later
    parser.add_argument<std::regex>({
        {ShortArgument, "g"},
        {LongArgument, "grep"},
        {HelpText, "Grep pattern"}
    });

    // A required flag 
    parser.add_argument<std::string>({
        {LongArgument, "file"},
        {Required, true},
        {HelpText, "File to grep"}
    });

    // Run action callback on complete 
    parser.on_complete([](argument_parser::base_parser const &p) {
        auto filename = p.get_optional<std::string>("file");
        auto pattern = p.get_optional<std::regex>("grep");
        
        if (filename && pattern) {
            std::cout << "Grepping " << filename.value() << " with pattern." << std::endl;
        }
    });

    // Register Conventions 
    const std::initializer_list<argument_parser::conventions::convention const *const> conventions = {
        &argument_parser::conventions::gnu_argument_convention,
        &argument_parser::conventions::windows_argument_convention
    };

    // Execute logic! 
    parser.handle_arguments(conventions);

    return 0;
}
```

### 2. Custom Type Parsing

You can natively parse your custom structs, objects, or arrays by specializing `argument_parser::parsing_traits::parser_trait<T>`.

```cpp
struct Point {
    int x, y;
};

template <> struct argument_parser::parsing_traits::parser_trait<Point> {
    static Point parse(const std::string &input) {
        auto comma_pos = input.find(',');
        int x = std::stoi(input.substr(0, comma_pos));
        int y = std::stoi(input.substr(comma_pos + 1));
        return {x, y};
    }
};

// Now you can directly use your type:
// parser.add_argument<Point>({ {LongArgument, "point"} });
// auto point = parser.get_optional<Point>("point");
```

## CMake Integration

The library can be installed globally via CMake or incorporated into your project.

```cmake
add_subdirectory(argument-parser)
target_link_libraries(your_target PRIVATE argument_parser)
```

## Building & Installing

```bash
mkdir build && cd build
cmake ..
cmake --build .
```
