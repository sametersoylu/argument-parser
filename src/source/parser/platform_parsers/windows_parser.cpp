
#ifdef _WIN32

#include "windows_parser.hpp"
#include "argument_parser.hpp"

#include <Windows.h>
#include <iostream>
#include <memory>
#include <shellapi.h>
#include <stdexcept>
#include <string>

using namespace std::string_literals;

struct local_free_deleter {
	void operator()(void *ptr) const {
		if (ptr == nullptr) {
			return;
		}
		LocalFree(static_cast<HLOCAL>(ptr));
	}
};

std::string windows_error_message(DWORD error_code) {
	LPSTR messageBuffer = nullptr;

	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

	if (size == 0 || !messageBuffer) {
		return "Unknown Error ("s + std::to_string(error_code) + ")";
	}

	std::unique_ptr<char, local_free_deleter> smartBuffer(messageBuffer);
	std::string result(smartBuffer.get(), size);
	result.erase(result.find_last_not_of(" \n\r\t") + 1);

	return result;
}

std::string utf8_from_wstring(const std::wstring &w) {
	if (w.empty())
		return {};

	int needed = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w.data(), static_cast<int>(w.size()), nullptr, 0,
									   nullptr, nullptr);
	if (needed <= 0) {
		throw std::runtime_error("WideCharToMultiByte sizing failed ("s + windows_error_message(::GetLastError()) +
								 ")");
	}
	std::string out;
	out.resize(needed);
	int written = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w.data(), static_cast<int>(w.size()), out.data(),
										needed, nullptr, nullptr);
	if (written <= 0) {
		throw std::runtime_error(
			"WideCharToMultiByte convert failed, Error("s + windows_error_message(::GetLastError()) + ")" +
			" Size (Needed): " + std::to_string(needed) + " Size (Written): " + std::to_string(written) +
			" Size (Allocated): " + std::to_string(out.size()));
	}
	return out;
}

void parse_windows_arguments(std::vector<std::string> &parsed_arguments,
							 std::function<void(std::string)> const &setProgramName) {
	int argc_w;
	std::unique_ptr<LPWSTR[], local_free_deleter> argv_w(CommandLineToArgvW(GetCommandLineW(), &argc_w));
	if (argv_w == nullptr) {
		throw std::runtime_error("CommandLineToArgvW failed");
	}

	if (argc_w <= 0) {
		return;
	}

	setProgramName(utf8_from_wstring(argv_w[0]));

	for (int i = 1; i < argc_w; i++) {
		try {
			std::string arg = utf8_from_wstring(argv_w[i]);
			parsed_arguments.emplace_back(arg);
		} catch (std::runtime_error e) {
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}
}

namespace argument_parser {
	windows_parser::windows_parser() {
		parse_windows_arguments(parsed_arguments,
								[this](std::string const &program_name) { this->program_name = program_name; });
	}
} // namespace argument_parser

namespace argument_parser::v2 {
	windows_parser::windows_parser(parser_settings const &settings) {
		parse_windows_arguments(ref_parsed_args(),
								[this](std::string const &program_name) { this->set_program_name(program_name); });

		prepare_help_flag(settings.should_exit_on_help);
	}
} // namespace argument_parser::v2

#endif
