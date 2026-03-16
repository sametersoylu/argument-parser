#ifdef _WIN32

#include "windows_parser.hpp"

#include <Windows.h>
#include <memory>
#include <shellapi.h>
#include <string>

std::string utf8_from_wstring(const std::wstring &w) {
	if (w.empty())
		return {};
	int needed = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (needed <= 0) {
		throw std::runtime_error("WideCharToMultiByte sizing failed (" + std::to_string(::GetLastError()) + ")");
	}
	std::string out;
	out.resize(static_cast<size_t>(needed - 1));
	int written =
		::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w.c_str(), -1, out.data(), needed - 1, nullptr, nullptr);
	if (written <= 0) {
		throw std::runtime_error("WideCharToMultiByte convert failed (" + std::to_string(::GetLastError()) + ")");
	}
	return out;
}

struct local_free_deleter {
	void operator()(void *ptr) const {
		if (ptr == nullptr) {
			return;
		}
		LocalFree(static_cast<HLOCAL>(ptr));
	}
};

void parse_windows_arguments(std::vector<std::string> &parsed_arguments) {
	int argc_w;
	std::unique_ptr<LPWSTR[], local_free_deleter> argv_w(CommandLineToArgvW(GetCommandLineW(), &argc_w));
	if (argv_w == nullptr) {
		throw std::runtime_error("CommandLineToArgvW failed");
	}

	for (int i = 0; i < argc_w; i++) {
		std::string arg = utf8_from_wstring(argv_w[i]);
		parsed_arguments.emplace_back(arg);
	}
}

namespace argument_parser {
	windows_parser::windows_parser() {
		parse_windows_arguments(parsed_arguments);
	}
} // namespace argument_parser

namespace argument_parser::v2 {
	windows_parser::windows_parser() {
		parse_windows_arguments(ref_parsed_args());
	}
} // namespace argument_parser::v2

#endif