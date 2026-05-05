
#include <argparse>
#include <gnu_argument_convention.hpp>
#include <macros.h>
#include <iostream>
#include <parser_v2.hpp>
#include <string>
#include <traits.hpp>

using argument = argument_parser::builder::argument<>;

using argument_parser::parsing_traits::hint_type;

auto echo(std::string const& s) -> void {
    std::cout << s << '\n';
}

using namespace argument_parser::parsing_traits;

constexpr hint_type vector_purpose_hint = "vector of ";

template<typename T>
class parser_trait<std::vector<T>> {
    public:
    static std::vector<T> parse(std::string const& s) {
        std::vector<T> result;
        std::stringstream ss(s);

        for (std::string line; std::getline(ss, line, ',');) {
            T item = parser_trait<T>::parse(line);
            result.push_back(item);
        }
        return result;
    }

    ARGPARSE_TRAIT_FORMAT_HINT = concat<
        hint_provider<&parser_trait<T>::format_hint>,
        hint_provider<&comma>,
        hint_provider<&parser_trait<T>::format_hint>
    >;

    ARGPARSE_TRAIT_PURPOSE_HINT = concat<
        hint_provider<&vector_purpose_hint>,
        hint_provider<&parser_trait<T>::purpose_hint>
    >;
};

using namespace argument_parser::v2::flags;

auto main() -> int {
    argument_parser::v2::parser parser(false);

    argument::start()
        .positional("count")
        .position(0)
        .help_text("How many times to repeat the action.")
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

    // parser.add_argument<int>({
    //     {ShortArgument, "c"},
    //     {HelpText, "capture count"},
    //     {Reference, &captured_value},
    // });

    argument::start()
        .short_argument("q")
        .help_text("Store a boolean flag.")
        .build(parser);

    // argument::start()
    //     .long_argument("regex")
    //     .help_text("Store a regex value.")
    //     .store<std::optional<std::regex>>()
    //     .build(parser);

    argument::start()
        .short_argument("e")
        .long_argument("echo")
        .help_text("Echo the parsed value.")
        .action(echo)
        .build(parser);

    argument::start()
        .long_argument("vecstr")
        .short_argument("vs")
        .action<std::vector<int>>([](std::vector<int> const& vecstr) {
            for (auto const& str : vecstr) {
                std::cout << str << '\n';
            }
        })
        .build(parser);

    parser.handle_arguments({
        &argument_parser::conventions::gnu_argument_convention
    });

    std::cout << "captured value: " << captured_value << '\n';

    return 0;
}
