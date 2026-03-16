#pragma once
#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include <any>
#include <atomic>
#include <base_convention.hpp>
#include <functional>
#include <initializer_list>
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <traits.hpp>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace argument_parser {
	namespace internal::sfinae {
		template <typename T> struct has_format_hint {
		private:
			typedef char YesType[1];
			typedef char NoType[2];

			template <typename C> static YesType &test(decltype(&C::format_hint));
			template <typename> static NoType &test(...);

		public:
			static constexpr bool value = sizeof(test<T>(0)) == sizeof(YesType);
		};

		template <typename T> struct has_purpose_hint {
		private:
			typedef char YesType[1];
			typedef char NoType[2];

			template <typename C> static YesType &test(decltype(&C::purpose_hint));
			template <typename> static NoType &test(...);

		public:
			static constexpr bool value = sizeof(test<T>(0)) == sizeof(YesType);
		};
	} // namespace internal::sfinae

	namespace internal::atomic {
		template <typename T> class copyable_atomic {
		public:
			copyable_atomic() : value(std::make_shared<std::atomic<T>>()) {}
			copyable_atomic(T desired) : value(std::make_shared<std::atomic<T>>(desired)) {}

			copyable_atomic(const copyable_atomic &other) : value(other.value) {}
			copyable_atomic &operator=(const copyable_atomic &other) {
				if (this != &other) {
					value = other.value;
				}
				return *this;
			}

			copyable_atomic(copyable_atomic &&other) noexcept = default;
			copyable_atomic &operator=(copyable_atomic &&other) noexcept = default;
			~copyable_atomic() = default;

			T operator=(T desired) noexcept {
				store(desired);
				return desired;
			}

			operator T() const noexcept {
				return load();
			}

			void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
				if (value) {
					value->store(desired, order);
				}
			}

			T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
				return value ? value->load(order) : T{};
			}

			T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
				return value ? value->exchange(desired, order) : T{};
			}

		private:
			std::shared_ptr<std::atomic<T>> value;
		};
	} // namespace internal::atomic

	class action_base {
	public:
		virtual ~action_base() = default;
		[[nodiscard]] virtual bool expects_parameter() const = 0;
		virtual void invoke() const = 0;
		virtual void invoke_with_parameter(const std::string &param) const = 0;
		[[nodiscard]] virtual std::pair<std::string, std::string> get_trait_hints() const = 0;
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
			bool parse_success = false;
			try {
				T parsed_value = parsing_traits::parser_trait<T>::parse(param);
				parse_success = true;
				invoke(parsed_value);
			} catch (const std::runtime_error &e) {
				if (!parse_success) {
					auto [format_hint, purpose_hint] = get_trait_hints();
					if (purpose_hint.empty())
						purpose_hint = "value";
					std::string error_text{"'" + param + "' is not a valid " + purpose_hint + " ${KEY}"};
					if (!format_hint.empty())
						error_text += "\nExpected format: " + format_hint;
					throw std::runtime_error(error_text);
				}
			}
		}

		[[nodiscard]] std::pair<std::string, std::string> get_trait_hints() const override {
			if constexpr (internal::sfinae::has_format_hint<parsing_traits::parser_trait<T>>::value &&
						  internal::sfinae::has_purpose_hint<parsing_traits::parser_trait<T>>::value) {
				return {parsing_traits::parser_trait<T>::format_hint, parsing_traits::parser_trait<T>::purpose_hint};
			} else {
				return {"", "value"};
			}
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

		[[nodiscard]] std::pair<std::string, std::string> get_trait_hints() const override {
			return {"", ""};
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

	/**
	 * @brief Base class for parsing arguments from the command line.
	 *
	 * Note: This class and its methods are NOT thread-safe.
	 * It must be instantiated and used from a single thread (typically the main thread),
	 * as operations such as argument processing and checking rely on thread-local or instance-specific state.
	 */
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

		void reset_current_conventions() {
			_current_conventions = {};
		}

		void current_conventions(std::initializer_list<conventions::convention const *const> convention_types) {
			_current_conventions = convention_types;
		}

		[[nodiscard]] std::initializer_list<conventions::convention const *const> current_conventions() const {
			return _current_conventions;
		}

	private:
		bool test_conventions(std::initializer_list<conventions::convention const *const> convention_types,
							  std::unordered_map<std::string, std::string> &values_for_arguments,
							  std::vector<std::pair<std::string, argument>> &found_arguments,
							  std::optional<argument> &found_help, std::vector<std::string>::iterator it,
							  std::stringstream &error_stream);
		void extract_arguments(std::initializer_list<conventions::convention const *const> convention_types,
							   std::unordered_map<std::string, std::string> &values_for_arguments,
							   std::vector<std::pair<std::string, argument>> &found_arguments,
							   std::optional<argument> &found_help);

		void invoke_arguments(std::unordered_map<std::string, std::string> const &values_for_arguments,
							  std::vector<std::pair<std::string, argument>> &found_arguments,
							  std::optional<argument> const &found_help);
		void enforce_creation_thread();

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

		std::initializer_list<conventions::convention const *const> _current_conventions;
		internal::atomic::copyable_atomic<std::thread::id> creation_thread_id = std::this_thread::get_id();

		std::list<std::function<void(base_parser const &)>> on_complete_events;

		friend class linux_parser;
		friend class windows_parser;
		friend class macos_parser;
		friend class fake_parser;
	};
} // namespace argument_parser

#endif // ARGUMENT_PARSER_HPP