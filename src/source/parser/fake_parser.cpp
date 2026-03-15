#include "fake_parser.hpp"

namespace argument_parser {
	fake_parser::fake_parser(std::string program_name, std::vector<std::string> const &arguments) {
		this->program_name = std::move(program_name);
		parsed_arguments = arguments;
	}

	fake_parser::fake_parser(std::string const &program_name, std::vector<std::string> &&arguments) {
		this->program_name = program_name;
		parsed_arguments = std::move(arguments);
	}

	fake_parser::fake_parser(std::string const &program_name, std::initializer_list<std::string> const &arguments)
		: fake_parser(program_name, std::vector<std::string>(arguments)) {}

	void fake_parser::set_program_name(std::string const &program_name) {
		this->program_name = program_name;
	}

	void fake_parser::set_parsed_arguments(std::vector<std::string> const &parsed_arguments) {
		this->parsed_arguments = parsed_arguments;
	}
} // namespace argument_parser