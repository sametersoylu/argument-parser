# argument-parser

A lightweight, modern, and highly customizable C++17 argument parser with native platform argument collection, trait-driven typed parsing, pluggable option conventions, and a fluent `v2` builder API.

> `v1` is deprecated and mainly kept as implementation history. For new projects, use `argument_parser::v2` together with `argument_parser::builder`.

## Features

- Native platform parser alias: `argument_parser::v2::parser` resolves to the current platform parser and reads arguments directly from OS APIs.
- Fluent builder API with compile-time builder constraints that prevent invalid combinations after a terminal/mutually exclusive mode has been selected.
- Type-safe parsing and extraction. Just extend `parser_trait<T>` for your types and if just want to store use `get_optional<T>()`!
- Positional arguments with optional explicit ordering and support for `--` as a positional separator.
- Trait-driven `format_hint` and `purpose_hint` metadata used in generated help text and parse errors.
- Automatic help flag on `argument_parser::v2::parser` (`-h`, `--help`) with configurable exit behavior.
- Auto-formatted help output..
- Completion hooks via `parser.on_complete(...)`.
- Pluggable conventions for GNU next-token, GNU equal-style, Windows next-token, and Windows inline `=` / `:` parsing, or bring your own!
- Testing helper + pseudo command handler `argument_parser::v2::fake_parser`.

## Requirements

- C++17 or later
- CMake 3.15 or later

## Quick Start

```cpp
#include <argparse>
#include <iostream>
#include <string>

using argument = argument_parser::builder::argument<>;

int main() {
    argument_parser::v2::parser parser(false); // --help prints without exiting immediately

    int threshold = 0;

    argument::start()
        .short_argument("e")
        .long_argument("echo")
        .action<std::string>([](std::string const& text) {
            std::cout << text << '\n';
        })
        .build(parser);

    argument::start()
        .long_argument("file")
        .store<std::string>()
        .required()
        .help_text("Input file to process.")
        .build(parser);

    argument::start()
        .long_argument("threshold")
        .reference(threshold)
        .build(parser);

    argument::start()
        .short_argument("v")
        .long_argument("verbose")
        .flag()
        .help_text("Enable verbose output.")
        .build(parser);

    argument::start()
        .positional("output")
        .position(0)
        .help_text("Output file.")
        .build(parser);

    parser.on_complete([](auto const& state) {
        if (auto file = state.template get_optional<std::string>("file")) {
            std::cout << "completed for: " << *file << '\n';
        }
    });

    const std::initializer_list<argument_parser::conventions::convention const *const> conventions = {
        &argument_parser::conventions::gnu_argument_convention,
        &argument_parser::conventions::gnu_equal_argument_convention,
        &argument_parser::conventions::windows_argument_convention,
        &argument_parser::conventions::windows_equal_argument_convention,
    };

    parser.handle_arguments(conventions);

    if (auto file = parser.get_optional<std::string>("file")) {
        std::cout << "file: " << *file << '\n';
    }

    std::cout << "threshold: " << threshold << '\n';
}
```

## Trait-Driven Parsing and Hints

Specialize `argument_parser::parsing_traits::parser_trait<T>` to add support for your own types and to describe their expected format.

```cpp
#include <macros.h>
#include <traits.hpp>
#include <stdexcept>
#include <string>

struct Point {
    int x;
    int y;
};

template <>
struct argument_parser::parsing_traits::parser_trait<Point> {
    static Point parse(std::string const& input) {
        auto comma = input.find(',');
        if (comma == std::string::npos) {
            throw std::runtime_error("Expected x,y");
        }

        return {
            std::stoi(input.substr(0, comma)),
            std::stoi(input.substr(comma + 1))
        };
    }

    ARGPARSE_TRAIT_FORMAT_HINT = "x,y";
    ARGPARSE_TRAIT_PURPOSE_HINT = "point coordinates";
};
```

Then use the type directly from the builder:

```cpp
argument::start()
    .long_argument("point")
    .store<Point>()
    .build(parser);
```

If you omit `help_text()`, `v2` uses the trait hints to generate help such as `Accepts point coordinates in x,y format.` The same hints are also included in type conversion errors.

## Help Behavior

`argument_parser::v2::parser` automatically registers `-h` and `--help`.

```cpp
argument_parser::v2::parser parser;       // help prints and exits
argument_parser::v2::parser parser(false); // help prints without immediate exit
```

You can also display help manually:

```cpp
parser.display_help(conventions);
```

## Supported Conventions

- GNU next-token: `-o value`, `--output value`
- GNU equal-style: `-o=value`, `--output=value`
- Windows next-token: `/output value`
- Windows inline value: `/output=value`, `/output:value`

Mix any of them in the same parser by passing the conventions you want to `handle_arguments()`.

## Builder Modes

`argument_parser::builder::argument<>` is a staged builder. `build(parser)` is the terminal call.

Before `build(...)`, you compose an argument from three kinds of steps:

- Identifier selection: `short_argument(...)`, `long_argument(...)`, or `positional(...)`
- Optional metadata: `position(...)` for positional arguments, `help_text(...)`, and `required(...)`
- One mutually exclusive value behavior:
  - `store<T>()` to parse and retain a value for later `get_optional<T>()`
  - `flag()` to store a boolean presence flag
  - `reference(value)` to write the parsed result directly into an existing variable
  - `action([] { ... })` for no-value callbacks
  - `action<T>([](T const&) { ... })` for typed value callbacks

Once you select one value behavior, the other value behavior methods are disabled at compile time, so combinations like `store<T>().action(...)` or `flag().reference(value)` are rejected by the type system. Also you cannot use the same method repeatedly as it is also disabled at compile time by the type system.

If you do not select a value behavior explicitly, `build(parser)` uses the default for the argument kind: named arguments become boolean flags, while positional arguments store a `std::string`.

## Testing

For unit tests or synthetic argument lists, use `argument_parser::v2::fake_parser` instead of the native platform parser:

```cpp
#include <fake_parser.hpp>

argument_parser::v2::fake_parser parser("tool", {"--count", "3", "input.txt"});
```

## CMake Integration

Use the project directly:

```cmake
add_subdirectory(argument-parser)
target_link_libraries(your_target PRIVATE argument_parser)
```

Or install and consume it as a package:

```cmake
find_package(argument_parser CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE argument_parser::argument_parser)
```

## Building & Installing

```bash
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```
