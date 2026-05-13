
#include <argparse>
#include <argument_parser.hpp>
#include <gnu_argument_convention.hpp>
#include <iostream>
#include <macros.h>
#include <parser_v2.hpp>
#include <string>
#include <traits.hpp>
#include <vector>
#include <windows_argument_convention.hpp>

using argument_parser::builder::new_argument;

using argument_parser::parsing_traits::hint_type;

auto echo(std::string const &s) -> void {
	std::cout << s << '\n';
}

using namespace argument_parser::parsing_traits;

constexpr hint_type vector_purpose_hint = "vector of ";
template <typename T> struct parser_trait<std::vector<T>> {
	static std::vector<T> parse(std::string const &s) {
		std::vector<T> result;
		std::stringstream ss(s);

		for (std::string line; std::getline(ss, line, ',');) {
			T item = parser_trait<T>::parse(line);
			result.push_back(item);
		}
		return result;
	}

	ARGPARSE_TRAIT_FORMAT_HINT = concat<hint_provider<&parser_trait<T>::format_hint>, hint_provider<&comma>,
										hint_provider<&parser_trait<T>::format_hint>>;

	ARGPARSE_TRAIT_PURPOSE_HINT =
		concat<hint_provider<&vector_purpose_hint>, hint_provider<&parser_trait<T>::purpose_hint>>;
};

using namespace argument_parser::v2::flags;

auto main() -> int {
	argument_parser::v2::parser parser(argument_parser::no_exit);

	new_argument()
		.positional("count")
		.position(1)
		.help_text("How many times to repeat the action.")
		.action<int>([](int const &count) { std::cout << "count action configured for " << count << '\n'; })
		.build(parser);

	int captured_value = 0;
	new_argument()
		.long_argument("threshold")
		.help_text("Store the parsed value through a reference.")
		.reference(captured_value)
		.build(parser);

	new_argument()
		.positional("captured")
		.position(0)
		.help_text("Store the parsed value through a reference.")
		.reference(captured_value)
		.build(parser);

	// parser.add_argument<int>({
	//     {ShortArgument, "c"},
	//     {HelpText, "capture count"},
	//     {Reference, &captured_value},
	// });

	new_argument().short_argument("q").help_text("Store a boolean flag.").build(parser);

	// argument::start()
	//     .long_argument("regex")
	//     .help_text("Store a regex value.")
	//     .store<std::optional<std::regex>>()
	//     .build(parser);

	new_argument()
		.short_argument("e")
		.long_argument("echo")
		.help_text("Echo the parsed value.")
		.action(echo)
		.build(parser);

	new_argument()
		.long_argument("vecstr")
		.short_argument("vs")
		.action<std::vector<int>>([](std::vector<int> const &vecstr) {
			for (auto const &str : vecstr) {
				std::cout << str << '\n';
			}
		})
		.build(parser);

	auto accumulated_pos = new_argument()
							   .short_argument("v")
							   .help_text("turns on verbose execution. up to three levels of verbosity.")
							   .count()
							   .build_and_get(parser);

	auto accumulate_vec =
		new_argument().long_argument("vecstr1").short_argument("vs1").accumulate<int>().build_and_get(parser);

	parser.add_argument<std::vector<int>>({
		{LongArgument, "accumulate"},
		{HelpText, "accumulates given ints into the vector (flag ver)"},
		{Accumulate, true},
	});

	std::vector<int> captured_vec;
	parser.add_argument<std::vector<int>>({
		{LongArgument, "accumulate2"},
		{HelpText, "accumulates given ints into the vector (ref ver)"},
		{Accumulate, &captured_vec},
	});

	std::vector<int> captured_vec2;
	parser.add_argument<std::vector<int>>({
		{LongArgument, "accumulate3"},
		{HelpText, "accumulates given ints into the vector (ref ver)"},
		{Accumulate, true},
		{Reference, &captured_vec2},
	});

	parser.on_complete([](argument_parser::base_parser const &p) {
		if (const auto value = p.get_optional<std::vector<int>>("accumulate"); value.has_value()) {
			std::cout << "accumulate: ";
			for (auto const &str : *value) {
				std::cout << str << '\n';
			}
		}
	});

	parser.handle_arguments({&argument_parser::conventions::gnu_argument_convention,
							 &argument_parser::conventions::windows_argument_convention});
	if (!captured_vec.empty()) {
		std::cout << "accumulate2: ";
		for (auto const &str : captured_vec) {
			std::cout << str << '\n';
		}
	}

	if (!captured_vec2.empty()) {
		std::cout << "accumulate3: ";
		for (auto const &str : captured_vec2) {
			std::cout << str << '\n';
		}
	}

	if (accumulate_vec) {
		std::cout << "accumulate_vec: ";
		for (auto const &str : *accumulate_vec) {
			std::cout << str << '\n';
		}
	}

	if (accumulated_pos) {
		std::cout << "accumulated_pos: " << *accumulated_pos << '\n';
	}

	return 0;
}
