#pragma once
#include <list>
#include <optional>
#include <type_traits>
#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP
#include <any>
#include <atomic>
#include <base_convention.hpp>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <traits.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace argument_parser {
	class action_base {
	public:
		virtual ~action_base() = default;
		[[nodiscard]] virtual bool expects_parameter() const = 0;
		virtual void invoke() const = 0;
		virtual void invoke_with_parameter(const std::string &param) const = 0;
		[[nodiscard]] virtual std::unique_ptr<action_base> clone() const = 0;
	};

	template <typename T> class parametered_action : public action_base {
	public:
		explicit parametered_action(std::function<void(const T &)> const &handler) : handler(handler) {}
		using parameter_type = T;
		void invoke(const T &arg) const {
			handler(arg);
		}

		[[nodiscard]] bool expects_parameter() const override {
			return true;
		}

		void invoke() const override {
			throw std::runtime_error("Parametered action requires a parameter");
		}

		void invoke_with_parameter(const std::string &param) const override {
			T parsed_value = parsing_traits::parser_trait<T>::parse(param);
			invoke(parsed_value);
		}

		[[nodiscard]] std::unique_ptr<action_base> clone() const override {
			return std::make_unique<parametered_action<T>>(handler);
		}

	private:
		std::function<void(const T &)> handler;
	};

	class non_parametered_action : public action_base {
	public:
		explicit non_parametered_action(std::function<void()> const &handler) : handler(handler) {}

		void invoke() const override {
			handler();
		}

		[[nodiscard]] bool expects_parameter() const override {
			return false;
		}

		void invoke_with_parameter(const std::string &param) const override {
			invoke();
		}

		[[nodiscard]] std::unique_ptr<action_base> clone() const override {
			return std::make_unique<non_parametered_action>(handler);
		}

	private:
		std::function<void()> handler;
	};

	class base_parser;

	class argument {
	public:
		argument();

		template <typename ActionType>
		argument(const int id, std::string name, ActionType const &action)
			: id(id), name(std::move(name)), action(action.clone()), required(false), invoked(false) {}

		argument(const argument &other);
		argument &operator=(const argument &other);
		argument(argument &&other) noexcept = default;
		argument &operator=(argument &&other) noexcept = default;

		[[nodiscard]] bool is_required() const;
		[[nodiscard]] std::string get_name() const;
		[[nodiscard]] bool is_invoked() const;
		[[nodiscard]] bool expects_parameter() const;
		[[nodiscard]] std::string get_help_text() const;

	private:
		void set_required(bool val);
		void set_invoked(bool val);
		void set_help_text(std::string const &text);

		friend class base_parser;

		int id;
		std::string name;
		std::unique_ptr<action_base> action;
		bool required;
		bool invoked;
		std::string help_text;
	};

	namespace helpers {
		template <typename T>
		static parametered_action<T> make_parametered_action(std::function<void(const T &)> const &function) {
			return parametered_action<T>(function);
		}

		static non_parametered_action make_non_parametered_action(std::function<void()> const &function) {
			return non_parametered_action(function);
		}
	} // namespace helpers

	class base_parser {
	public:
		template <typename T>
		void add_argument(std::string const &short_arg, std::string const &long_arg, std::string const &help_text,
						  parametered_action<T> const &action, bool required) {
			base_add_argument(short_arg, long_arg, help_text, action, required);
		}

		template <typename T>
		void add_argument(std::string const &short_arg, std::string const &long_arg, std::string const &help_text,
						  bool required) {
			base_add_argument<T>(short_arg, long_arg, help_text, required);
		}

		void add_argument(std::string const &short_arg, std::string const &long_arg, std::string const &help_text,
						  non_parametered_action const &action, bool required) {
			base_add_argument(short_arg, long_arg, help_text, action, required);
		}

		void add_argument(std::string const &short_arg, std::string const &long_arg, std::string const &help_text,
						  bool required) {
			base_add_argument<void>(short_arg, long_arg, help_text, required);
		}

		void on_complete(std::function<void(base_parser const &)> const &action);

		template <typename T> std::optional<T> get_optional(std::string const &arg) const {
			auto id = find_argument_id(arg);
			if (id.has_value()) {
				auto value = stored_arguments.find(id.value());
				if (value != stored_arguments.end() && value->second.has_value()) {
					return std::any_cast<T>(value->second);
				}
			}
			return std::nullopt;
		}

		[[nodiscard]] std::string
		build_help_text(std::initializer_list<conventions::convention const *const> convention_types) const;
		argument &get_argument(conventions::parsed_argument const &arg);
		[[nodiscard]] std::optional<int> find_argument_id(std::string const &arg) const;
		void handle_arguments(std::initializer_list<conventions::convention const *const> convention_types);
		void display_help(std::initializer_list<conventions::convention const *const> convention_types) const;

	protected:
		base_parser() = default;

		std::string program_name;
		std::vector<std::string> parsed_arguments;

	private:
		void assert_argument_not_exist(std::string const &short_arg, std::string const &long_arg) const;
		static void set_argument_status(bool is_required, std::string const &help_text, argument &arg);
		void place_argument(int id, argument const &arg, std::string const &short_arg, std::string const &long_arg);

		template <typename ActionType>
		void base_add_argument(std::string const &short_arg, std::string const &long_arg, std::string const &help_text,
							   ActionType const &action, bool required) {
			assert_argument_not_exist(short_arg, long_arg);
			int id = id_counter.fetch_add(1);
			argument arg(id, short_arg + "|" + long_arg, action);
			set_argument_status(required, help_text, arg);
			place_argument(id, arg, short_arg, long_arg);
		}

		template <typename StoreType = void>
		void base_add_argument(std::string const &short_arg, std::string const &long_arg, std::string const &help_text,
							   bool required) {
			assert_argument_not_exist(short_arg, long_arg);
			int id = id_counter.fetch_add(1);
			if constexpr (std::is_same_v<StoreType, void>) {
				auto action =
					helpers::make_non_parametered_action([id, this] { stored_arguments[id] = std::any{true}; });
				argument arg(id, short_arg + "|" + long_arg, action);
				set_argument_status(required, help_text, arg);
				place_argument(id, arg, short_arg, long_arg);
			} else {
				auto action = helpers::make_parametered_action<StoreType>(
					[id, this](StoreType const &value) { stored_arguments[id] = std::any{value}; });
				argument arg(id, short_arg + "|" + long_arg, action);
				set_argument_status(required, help_text, arg);
				place_argument(id, arg, short_arg, long_arg);
			}
		}

		void check_for_required_arguments(std::initializer_list<conventions::convention const *const> convention_types);
		void fire_on_complete_events() const;

		inline static std::atomic_int id_counter = 0;

		std::unordered_map<int, std::any> stored_arguments;
		std::unordered_map<int, argument> argument_map;
		std::unordered_map<std::string, int> short_arguments;
		std::unordered_map<int, std::string> reverse_short_arguments;
		std::unordered_map<std::string, int> long_arguments;
		std::unordered_map<int, std::string> reverse_long_arguments;

		std::list<std::function<void(base_parser const &)>> on_complete_events;

		friend class linux_parser;
		friend class windows_parser;
		friend class macos_parser;
		friend class fake_parser;
	};
} // namespace argument_parser

#endif // ARGUMENT_PARSER_HPP