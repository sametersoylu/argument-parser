# argument-parser

A lightweight, modern, and highly customizable C++17 argument parser with native platform argument collection, trait-driven typed parsing, repeatable argument accumulation, pluggable option conventions, and a fluent `v2` builder API.

> `v1` is deprecated and mainly kept as implementation history. For new projects, use `argument_parser::v2` together with `argument_parser::builder`.

## Features

- Native platform parser alias: `argument_parser::v2::parser` resolves to the current platform parser and reads arguments directly from OS APIs.
- Fluent builder API with compile-time builder constraints that prevent invalid combinations after a terminal/mutually exclusive mode has been selected.
- Type-safe parsing and extraction. Extend `parser_trait<T>` for your own types and retrieve stored values with `get_optional<T>()`.
- Repeatable value accumulation with `accumulate<T>()`, `accumulate(vector&)`, `count()`, and `count(int&)`.
- `build_and_get(parser)` for storable builder modes, returning a small container that is populated after parsing completes.
- Positional arguments with optional explicit ordering and support for `--` as a positional separator.
- Trait-driven `format_hint` and `purpose_hint` metadata used in generated help text and parse errors.
- Automatic help flag on `argument_parser::v2::parser` (`-h`, `--help`) with configurable exit behavior through `parser_settings`.
- `parser_settings` for choosing whether help, unknown arguments, parse errors, and missing required arguments exit or throw.
- Auto-formatted help output.
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
#include <vector>

using argument_parser::builder::new_argument;

int main() {
    argument_parser::v2::parser parser(argument_parser::no_exit); // throw instead of exiting

    int threshold = 0;
    std::vector<int> ids;

    new_argument()
        .short_argument("e")
        .long_argument("echo")
        .action<std::string>([](std::string const& text) {
            std::cout << text << '\n';
        })
        .build(parser);

    new_argument()
        .long_argument("file")
        .store<std::string>()
        .required()
        .help_text("Input file to process.")
        .build(parser);

    new_argument()
        .long_argument("threshold")
        .reference(threshold)
        .help_text("Numeric threshold.")
        .build(parser);

    auto verbose = new_argument()
        .short_argument("v")
        .help_text("Increase verbosity. Repeat for a higher level.")
        .count()
        .build_and_get(parser);

    new_argument()
        .long_argument("id")
        .help_text("Collect an id. May be repeated.")
        .accumulate(ids)
        .build(parser);

    new_argument()
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
    std::cout << "ids: " << ids.size() << '\n';

    if (verbose) {
        std::cout << "verbosity: " << *verbose << '\n';
    }
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
new_argument()
    .long_argument("point")
    .store<Point>()
    .build(parser);
```

If you omit `help_text()`, `v2` uses the trait hints to generate help such as `Accepts point coordinates in x,y format.` The same hints are also included in type conversion errors.

## Help Behavior

`argument_parser::v2::parser` automatically registers `-h` and `--help`.

```cpp
argument_parser::v2::parser parser;                         // help prints and exits
argument_parser::v2::parser parser(argument_parser::no_exit); // help prints without exiting
```

You can also display help manually:

```cpp
parser.display_help(conventions);
```

## Parser Settings

Use `argument_parser::parser_settings` when the parser is embedded in a larger
application, test, or REPL-like tool where parse failures should be handled by
the caller instead of ending the process.

```cpp
argument_parser::parser_settings settings;
settings.should_exit_on_help = false;
settings.should_exit_on_error = false;
settings.should_exit_on_unknown_argument = false;
settings.should_exit_on_missing_required = false;
settings.ignore_unknown_arguments = true;

argument_parser::v2::parser parser(settings);
```

The convenience constant `argument_parser::no_exit` disables the four automatic
exit paths while leaving unknown-argument handling enabled:

```cpp
argument_parser::v2::parser parser(argument_parser::no_exit);
```

Settings fields:

- `should_exit_on_help`: exit with status `0` after automatic `-h` / `--help`.
- `should_exit_on_error`: exit with status `1` for non-unknown parse errors; otherwise rethrow.
- `should_exit_on_unknown_argument`: exit with status `1` for unknown arguments; otherwise rethrow.
- `should_exit_on_missing_required`: exit with status `1` after reporting missing required arguments; otherwise throw.
- `ignore_unknown_arguments`: skip unknown arguments that cannot be consumed as positionals.
- `ignore_errors`: present in the public settings struct, but not currently read by the parser runtime.

## Supported Conventions

- GNU next-token: `-o value`, `--output value`
- GNU equal-style: `-o=value`, `--output=value`
- Windows next-token: `/output value`
- Windows inline value: `/output=value`, `/output:value`

Mix any of them in the same parser by passing the conventions you want to `handle_arguments()`.

## Builder API

`argument_parser::builder::argument<>` is a staged builder. Prefer `argument_parser::builder::new_argument()` as the entry point:

```cpp
using argument_parser::builder::new_argument;
```

`build(parser)` registers the argument and returns `void`. `build_and_get(parser)` is available for storable modes and returns a lightweight `builder::container<T>`. The container is filled by an internal completion hook after `handle_arguments(...)` runs.

Before `build(...)`, you compose an argument from three kinds of steps:

- Identifier selection: `short_argument(...)`, `long_argument(...)`, or `positional(...)`
- Optional metadata: `position(...)` for positional arguments, `help_text(...)`, and `required(...)`
- One mutually exclusive value behavior:
  - `store<T>()` to parse and retain a value for later `get_optional<T>()`
  - `flag()` to store a boolean presence flag
  - `reference(value)` to write the parsed result directly into an existing variable
  - `accumulate<T>()` to collect repeated values into a stored `std::vector<T>`
  - `accumulate(vector)` to collect repeated values into an existing `std::vector<T>`
  - `count()` to store how many times an option appears
  - `count(value)` to write the occurrence count into an existing `int`
  - `action([] { ... })` for no-value callbacks
  - `action<T>([](T const&) { ... })` for typed value callbacks

Once you select one value behavior, the other value behavior methods are disabled at compile time, so combinations like `store<T>().action(...)` or `flag().reference(value)` are rejected by the type system. The same staged typing also prevents repeating one-shot methods such as `help_text(...)`, `position(...)`, or `store<T>()` on the same builder chain.

If you do not select a value behavior explicitly, `build(parser)` uses the default for the argument kind: named arguments become boolean flags, while positional arguments store a `std::string`.

## Accumulators and Counts

Use `accumulate<T>()` when the parser should accept the same value-bearing argument multiple times and store all parsed values:

```cpp
auto values = new_argument()
    .short_argument("n")
    .long_argument("number")
    .accumulate<int>()
    .build_and_get(parser);

parser.handle_arguments(conventions);

if (values) {
    for (int value : *values) {
        std::cout << value << '\n';
    }
}
```

Use `accumulate(target)` to append into a vector you own:

```cpp
std::vector<int> ids;

new_argument()
    .long_argument("id")
    .accumulate(ids)
    .build(parser);
```

Use `count()` for repeatable flags such as `-v -v -v`:

```cpp
auto verbosity = new_argument()
    .short_argument("v")
    .count()
    .build_and_get(parser);
```

The lower-level `v2::base_parser::add_argument` API exposes the same accumulator behavior through the `Accumulate` flag:

```cpp
using namespace argument_parser::v2::flags;

parser.add_argument<std::vector<int>>({
    {LongArgument, "id"},
    {HelpText, "Collect an id. May be repeated."},
    {Accumulate, true},
});

std::vector<int> captured_ids;
parser.add_argument<std::vector<int>>({
    {LongArgument, "captured-id"},
    {Accumulate, &captured_ids},
});
```

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
