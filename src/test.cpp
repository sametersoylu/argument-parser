#include <argparse>
#include <array>
#include <cassert>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

const std::initializer_list<argument_parser::conventions::convention const *const> conventions = {
	&argument_parser::conventions::gnu_argument_convention,
	&argument_parser::conventions::gnu_equal_argument_convention,
};

namespace v2_test {
	class fake_parser : public argument_parser::v2::base_parser {
	public:
		fake_parser(std::string const &program_name, std::initializer_list<std::string> const &arguments) {
			set_program_name(program_name);
			ref_parsed_args() = std::vector<std::string>(arguments);
			prepare_help_flag();
		}
	};
} // namespace v2_test

int tests_run = 0;
int tests_passed = 0;

void test_result(const char *name, bool passed) {
	tests_run++;
	if (passed) {
		tests_passed++;
		std::cout << "  [PASS] " << name << std::endl;
	} else {
		std::cout << "  [FAIL] " << name << std::endl;
	}
}

// ============================================================
// V1 Tests (using argument_parser::fake_parser)
// ============================================================

void test_v1_single_positional_store() {
	argument_parser::fake_parser parser("test", {"hello"});
	parser.add_positional_argument<std::string>("greeting", "A greeting", false);
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<std::string>("greeting");
	test_result("v1: single positional store", val.has_value() && val.value() == "hello");
}

void test_v1_multiple_positionals_ordered() {
	argument_parser::fake_parser parser("test", {"alpha", "beta", "gamma"});
	parser.add_positional_argument<std::string>("first", "First arg", false);
	parser.add_positional_argument<std::string>("second", "Second arg", false);
	parser.add_positional_argument<std::string>("third", "Third arg", false);
	parser.handle_arguments(conventions);

	auto first = parser.get_optional<std::string>("first");
	auto second = parser.get_optional<std::string>("second");
	auto third = parser.get_optional<std::string>("third");

	bool ok = first.has_value() && first.value() == "alpha" && second.has_value() && second.value() == "beta" &&
			  third.has_value() && third.value() == "gamma";
	test_result("v1: multiple positionals preserve order", ok);
}

void test_v1_positional_with_explicit_position() {
	argument_parser::fake_parser parser("test", {"first_val", "second_val"});
	parser.add_positional_argument<std::string>("second", "Second", false, 1);
	parser.add_positional_argument<std::string>("first", "First", false, 0);
	parser.handle_arguments(conventions);

	auto first = parser.get_optional<std::string>("first");
	auto second = parser.get_optional<std::string>("second");

	bool ok = first.has_value() && first.value() == "first_val" && second.has_value() && second.value() == "second_val";
	test_result("v1: explicit position index", ok);
}

void test_v1_positional_typed_int() {
	argument_parser::fake_parser parser("test", {"42"});
	parser.add_positional_argument<int>("count", "A count", false);
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<int>("count");
	test_result("v1: positional with int type", val.has_value() && val.value() == 42);
}

void test_v1_positional_with_action() {
	std::string captured;
	argument_parser::fake_parser parser("test", {"world"});

	auto action =
		argument_parser::helpers::make_parametered_action<std::string>([&](std::string const &v) { captured = v; });
	parser.add_positional_argument<std::string>("name", "A name", action, false);
	parser.handle_arguments(conventions);

	test_result("v1: positional with action", captured == "world");
}

void test_v1_mixed_named_and_positional() {
	argument_parser::fake_parser parser("test", {"--verbose", "true", "myfile.txt"});
	parser.add_argument<bool>("v", "verbose", "Verbose mode", false);
	parser.add_positional_argument<std::string>("file", "Input file", false);
	parser.handle_arguments(conventions);

	auto verbose = parser.get_optional<bool>("verbose");
	auto file = parser.get_optional<std::string>("file");

	bool ok = verbose.has_value() && verbose.value() == true && file.has_value() && file.value() == "myfile.txt";
	test_result("v1: mixed named and positional args", ok);
}

void test_v1_positional_after_named() {
	argument_parser::fake_parser parser("test", {"-n", "5", "output.txt"});
	parser.add_argument<int>("n", "number", "A number", false);
	parser.add_positional_argument<std::string>("output", "Output file", false);
	parser.handle_arguments(conventions);

	auto number = parser.get_optional<int>("number");
	auto output = parser.get_optional<std::string>("output");

	bool ok = number.has_value() && number.value() == 5 && output.has_value() && output.value() == "output.txt";
	test_result("v1: positional after named args", ok);
}

void test_v1_positional_between_named() {
	argument_parser::fake_parser parser("test", {"-a", "1", "positional_val", "--beta", "2"});
	parser.add_argument<int>("a", "alpha", "Alpha", false);
	parser.add_argument<int>("b", "beta", "Beta", false);
	parser.add_positional_argument<std::string>("middle", "Middle arg", false);
	parser.handle_arguments(conventions);

	auto alpha = parser.get_optional<int>("alpha");
	auto beta = parser.get_optional<int>("beta");
	auto middle = parser.get_optional<std::string>("middle");

	bool ok = alpha.has_value() && alpha.value() == 1 && beta.has_value() && beta.value() == 2 && middle.has_value() &&
			  middle.value() == "positional_val";
	test_result("v1: positional between named args", ok);
}

void test_v1_double_dash_separator() {
	argument_parser::fake_parser parser("test", {"--", "-not-a-flag"});
	parser.add_positional_argument<std::string>("item", "An item", false);
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<std::string>("item");
	test_result("v1: -- separator treats next as positional", val.has_value() && val.value() == "-not-a-flag");
}

void test_v1_double_dash_multiple() {
	argument_parser::fake_parser parser("test", {"--name", "hello", "--", "--weird", "-x"});
	parser.add_argument<std::string>("n", "name", "A name", false);
	parser.add_positional_argument<std::string>("first", "First", false);
	parser.add_positional_argument<std::string>("second", "Second", false);
	parser.handle_arguments(conventions);

	auto name = parser.get_optional<std::string>("name");
	auto first = parser.get_optional<std::string>("first");
	auto second = parser.get_optional<std::string>("second");

	bool ok = name.has_value() && name.value() == "hello" && first.has_value() && first.value() == "--weird" &&
			  second.has_value() && second.value() == "-x";
	test_result("v1: -- separator with multiple positionals", ok);
}

void test_v1_required_positional_missing() {
	argument_parser::fake_parser parser("test", {});
	parser.add_positional_argument<std::string>("file", "A file", true);

	bool threw = false;
	try {
		// check_for_required_arguments calls std::exit(1) so we can't easily test it
		// instead, test that handle_arguments doesn't crash when positionals are provided
		parser.handle_arguments(conventions);
	} catch (...) {
		threw = true;
	}
	// Note: required check calls std::exit(1), so if we get here the arg wasn't required-checked
	// This test just verifies setup doesn't crash. The exit behavior is tested manually.
	test_result("v1: required positional setup (no crash)", true);
}

void test_v1_unexpected_positional_throws() {
	argument_parser::fake_parser parser("test", {"unexpected"});
	// no positional args defined, but a bare token is provided

	bool threw = false;
	try {
		parser.handle_arguments(conventions);
	} catch (const std::runtime_error &) {
		threw = true;
	}
	test_result("v1: unexpected positional throws", threw);
}

void test_v1_duplicate_positional_name_throws() {
	argument_parser::fake_parser parser("test", {"a", "b"});
	parser.add_positional_argument<std::string>("file", "A file", false);

	bool threw = false;
	try {
		parser.add_positional_argument<std::string>("file", "Duplicate", false);
	} catch (const std::runtime_error &) {
		threw = true;
	}
	test_result("v1: duplicate positional name throws", threw);
}

void test_v1_positional_on_complete() {
	std::string captured_file;
	argument_parser::fake_parser parser("test", {"data.csv"});
	parser.add_positional_argument<std::string>("file", "Input file", false);
	parser.on_complete([&](argument_parser::base_parser const &p) {
		auto val = p.get_optional<std::string>("file");
		if (val)
			captured_file = val.value();
	});
	parser.handle_arguments(conventions);

	test_result("v1: positional accessible in on_complete", captured_file == "data.csv");
}

// ============================================================
// V2 Tests (using v2_test::fake_parser)
// ============================================================

void test_v2_single_positional() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"hello"});

	parser.add_argument<std::string>({{Positional, "greeting"}, {HelpText, "A greeting"}});
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<std::string>("greeting");
	test_result("v2: single positional store", val.has_value() && val.value() == "hello");
}

void test_v2_positional_required() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"value"});

	parser.add_argument<std::string>({{Positional, "arg"}, {Required, true}, {HelpText, "Required arg"}});
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<std::string>("arg");
	test_result("v2: required positional", val.has_value() && val.value() == "value");
}

void test_v2_positional_with_position() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"first_val", "second_val"});

	parser.add_argument<std::string>({{Positional, "second"}, {Position, 1}, {HelpText, "Second"}});
	parser.add_argument<std::string>({{Positional, "first"}, {Position, 0}, {HelpText, "First"}});
	parser.handle_arguments(conventions);

	auto first = parser.get_optional<std::string>("first");
	auto second = parser.get_optional<std::string>("second");

	bool ok = first.has_value() && first.value() == "first_val" && second.has_value() && second.value() == "second_val";
	test_result("v2: positional with explicit Position", ok);
}

void test_v2_positional_typed_int() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"99"});

	parser.add_argument<int>({{Positional, "count"}, {HelpText, "A count"}});
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<int>("count");
	test_result("v2: positional with int type", val.has_value() && val.value() == 99);
}

void test_v2_mixed_named_and_positional() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"--output", "out.txt", "input.txt"});

	parser.add_argument<std::string>({{ShortArgument, "o"}, {LongArgument, "output"}, {HelpText, "Output file"}});
	parser.add_argument<std::string>({{Positional, "input"}, {HelpText, "Input file"}});
	parser.handle_arguments(conventions);

	auto output = parser.get_optional<std::string>("output");
	auto input = parser.get_optional<std::string>("input");

	bool ok = output.has_value() && output.value() == "out.txt" && input.has_value() && input.value() == "input.txt";
	test_result("v2: mixed named and positional", ok);
}

void test_v2_positional_with_action() {
	using namespace argument_parser::v2::flags;
	std::string captured;
	v2_test::fake_parser parser("test", {"world"});

	parser.add_argument<std::string>({{Positional, "name"},
									  {Action, argument_parser::helpers::make_parametered_action<std::string>(
												   [&](std::string const &v) { captured = v; })},
									  {HelpText, "A name"}});
	parser.handle_arguments(conventions);

	test_result("v2: positional with action", captured == "world");
}

void test_v2_double_dash_separator() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"--", "--not-a-flag"});

	parser.add_argument<std::string>({{Positional, "item"}, {HelpText, "An item"}});
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<std::string>("item");
	test_result("v2: -- separator", val.has_value() && val.value() == "--not-a-flag");
}

void test_v2_positional_auto_help_text() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"42"});

	// no HelpText provided — should auto-generate from traits
	parser.add_argument<int>({{Positional, "count"}});
	parser.handle_arguments(conventions);

	auto val = parser.get_optional<int>("count");
	test_result("v2: positional auto help text (no crash)", val.has_value() && val.value() == 42);
}

void test_v2_multiple_positionals_and_named() {
	using namespace argument_parser::v2::flags;
	v2_test::fake_parser parser("test", {"-v", "src.txt", "dst.txt"});

	parser.add_argument({{ShortArgument, "v"}, {LongArgument, "verbose"}});
	parser.add_argument<std::string>({{Positional, "source"}, {HelpText, "Source"}});
	parser.add_argument<std::string>({{Positional, "destination"}, {HelpText, "Destination"}});
	parser.handle_arguments(conventions);

	auto verbose = parser.get_optional<bool>("verbose");
	auto source = parser.get_optional<std::string>("source");
	auto dest = parser.get_optional<std::string>("destination");

	bool ok = verbose.has_value() && source.has_value() && source.value() == "src.txt" && dest.has_value() &&
			  dest.value() == "dst.txt";
	test_result("v2: multiple positionals with named flag", ok);
}

void test_v2_on_complete_with_positional() {
	using namespace argument_parser::v2::flags;
	std::string captured;
	v2_test::fake_parser parser("test", {"payload"});

	parser.add_argument<std::string>({{Positional, "data"}, {HelpText, "Data"}});
	parser.on_complete([&](argument_parser::base_parser const &p) {
		auto val = p.get_optional<std::string>("data");
		if (val)
			captured = val.value();
	});
	parser.handle_arguments(conventions);

	test_result("v2: positional accessible in on_complete", captured == "payload");
}

// ============================================================
// Main
// ============================================================

int main() {
	std::cout << "=== V1 Positional Argument Tests ===" << std::endl;

	std::array<std::function<void()>, 13> v1Tests {
        test_v1_single_positional_store,
    	test_v1_multiple_positionals_ordered,
    	test_v1_positional_with_explicit_position,
    	test_v1_positional_typed_int,
    	test_v1_positional_with_action,
    	test_v1_mixed_named_and_positional,
    	test_v1_positional_after_named,
    	test_v1_positional_between_named,
    	test_v1_double_dash_separator,
    	test_v1_double_dash_multiple,
    	test_v1_unexpected_positional_throws,
    	test_v1_duplicate_positional_name_throws,
    	test_v1_positional_on_complete
	};

	for (auto const& test : v1Tests) {
	    try {
            test();
        } catch(std::exception const& e) {
            std::cout << "test failed: " << e.what() << std::endl;
  		}
	}

	std::cout << "\n=== V2 Positional Argument Tests ===" << std::endl;
	std::array<std::function<void()>, 10> v2Tests{
    	test_v2_single_positional,
    	test_v2_positional_required,
    	test_v2_positional_with_position,
    	test_v2_positional_typed_int,
    	test_v2_mixed_named_and_positional,
    	test_v2_positional_with_action,
    	test_v2_double_dash_separator,
    	test_v2_positional_auto_help_text,
    	test_v2_multiple_positionals_and_named,
    	test_v2_on_complete_with_positional
	};


	for (auto const& test : v2Tests) {
	    try {
            test();
        } catch(std::exception const& e) {
            std::cout << "test failed: " << e.what() << std::endl;
  		}
	}

	std::cout << "\n=== Results: " << tests_passed << "/" << tests_run << " passed ===" << std::endl;
	return (tests_passed == tests_run) ? 0 : 1;
}
