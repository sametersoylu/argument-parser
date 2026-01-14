#pragma once
#ifdef _WIN32
#include <argument_parser.hpp>
#include <memory>
#include <string>
#include <windows.h>
#include <shellapi.h>

namespace argument_parser {

class windows_parser : public base_parser {
public:
    windows_parser() {
        int nArgs = 0;
        LPWSTR* raw = ::CommandLineToArgvW(::GetCommandLineW(), &nArgs);
        if (!raw) {
            throw std::runtime_error("CommandLineToArgvW failed (" +
                                     std::to_string(::GetLastError()) + ")");
        }
        std::unique_ptr<LPWSTR, void(*)(LPWSTR*)> argvW{ raw, [](LPWSTR* p){ if (p) ::LocalFree(p); } };

        if (nArgs <= 0) {
            throw std::runtime_error("No command line arguments found.");
        }

        {
            std::wstring w0 = argvW.get()[0];
            auto pos = w0.find_last_of(L"\\/");
            std::wstring base = (pos == std::wstring::npos) ? w0 : w0.substr(pos + 1);
            program_name = utf8_from_wstring(base);
        }

        parsed_arguments.reserve(static_cast<size_t>(nArgs > 0 ? nArgs - 1 : 0));
        for (int i = 1; i < nArgs; ++i) {
            parsed_arguments.emplace_back(utf8_from_wstring(argvW.get()[i]));
        }
    }

private:
    static std::string utf8_from_wstring(const std::wstring& w) {
        if (w.empty()) return {};
        int needed = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                           w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (needed <= 0) {
            throw std::runtime_error("WideCharToMultiByte sizing failed (" +
                                     std::to_string(::GetLastError()) + ")");
        }
        std::string out;
        out.resize(static_cast<size_t>(needed - 1));
        int written = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                            w.c_str(), -1, out.data(), needed - 1, nullptr, nullptr);
        if (written <= 0) {
            throw std::runtime_error("WideCharToMultiByte convert failed (" +
                                     std::to_string(::GetLastError()) + ")");
        }
        return out;
    }
};

}
#endif