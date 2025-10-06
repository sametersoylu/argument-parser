#pragma once
#ifdef _WIN32
#ifndef WINDOWS_PARSER_HPP

// compiler bitches if you don't use lean and mean and remove shit ton of other macros that comes preloaded with the windows api. 
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// I'm speaking to you Microsoft, STOP MAKING EVERYTHING A FUCKING MACRO. IT BREAKES CPP!
#define NOGDI
#define NOHELP
#define NOMCX
#define NOIME
#define NOCOMM
#define NOKANJI
#define NOSERVICE
#define NOMDI
#define NOSOUND

// also this fixes error about no architecture being targeted (somehow)
#if defined(_M_AMD64) && !defined(_AMD64_)
#  define _AMD64_
#endif
#if defined(_M_IX86) && !defined(_X86_)
#  define _X86_
#endif
#if defined(_M_ARM64) && !defined(_ARM64_)
#  define _ARM64_
#endif

#include <argument_parser.hpp>
#include <fstream>
#include <string>

// THIS HAS TO BE THE FIRST. DON'T CHANGE THEIR ORDER. 
#include <windows.h>
#include <processenv.h>
#include <shellapi.h>

namespace argument_parser {
    class windows_parser : public base_parser {
        public: 
        windows_parser() {
            int nArgs; 
            LPWSTR commandLineA = GetCommandLineW();
            auto argvW = std::unique_ptr<LPWSTR, local_free_deleter> { CommandLineToArgvW(commandLineA, &nArgs) };

            if (argvW == nullptr) {
				throw std::runtime_error("Failed to get command line arguments.");
            }

            if (nArgs <= 0) {
                throw std::runtime_error("No command line arguments found, including program name.");
			}

            program_name = utf8_from_wstring(argvW.get()[0]);
			parsed_arguments.reserve(static_cast<size_t>(nArgs - 1));
            for (int i = 1; i < nArgs; ++i) {
                parsed_arguments.emplace_back(utf8_from_wstring(argvW.get()[i]));
			}
        }

        private: 
        struct local_free_deleter {
            void operator()(HLOCAL ptr) const noexcept {
                if (ptr != nullptr) ::LocalFree(ptr);
            }
		};

        static std::string utf8_from_wstring(const std::wstring& w) {
			if (w.empty()) return {};
            int needed = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (needed == 0) {
                throw std::runtime_error("WideCharToMultiByte sizing failed ("
                    + std::to_string(GetLastError()) + ")");
            }

            std::string out;
            out.resize(static_cast<size_t>(needed - 1)); 
            int written = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1,
                out.data(), needed, nullptr, nullptr);
            if (written == 0) {
                throw std::runtime_error("WideCharToMultiByte convert failed ("
                    + std::to_string(GetLastError()) + ")");
            }
            return out;
        }
    };

    using parser = windows_parser; 
}

#endif 
#endif