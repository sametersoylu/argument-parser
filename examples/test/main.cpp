#include <argparse>
#include <gnu_argument_convention.hpp>
#include <parser_v3.hpp>
#include <iostream>
#include <regex>

using argument = argument_parser::builder::argument<>;

auto echo(std::string const& s) -> void {
    std::cout << s << '\n';
}

auto main() -> int {
    argument_parser::v2::parser parser(false);

    argument::start()
        .positional("count")
        .position(0)
        .help_text("How many times to repeat the action.")
        .required()
        .action<int>([](int const& count) {
            std::cout << "count action configured for " << count << '\n';
        })
        .build(parser);

    int captured_value = 0;
    argument::start()
        .long_argument("threshold")
        .help_text("Store the parsed value through a reference.")
        .reference(captured_value)
        .build(parser);

    argument::start()
        .short_argument("q")
        .help_text("Store a boolean flag.")
        .build(parser);

    argument::start()
        .long_argument("output")
        .help_text("Store a string value.")
        .store<std::regex>()
        .build(parser);

    argument::start()
        .short_argument("e")
        .long_argument("echo")
        .help_text("Echo the parsed value.")
        .action(echo)
        .build(parser);

    parser.handle_arguments({
        &argument_parser::conventions::gnu_argument_convention
    });

    return 0;
}
