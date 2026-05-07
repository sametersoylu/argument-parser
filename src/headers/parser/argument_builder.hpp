#pragma once

#include "argument_parser.hpp"
#include <functional>
#include <parser_v2.hpp>
#include <type_traits>

#ifndef ARGUMENT_PARSER_PARSER_V3_HPP
#define ARGUMENT_PARSER_PARSER_V3_HPP

namespace argument_parser::builder {

	class non_type {};
	namespace builder_mask {
		using v2_flag = argument_parser::v2::add_argument_flags;
		using mask_type = std::uint64_t;
		enum class value_mode { unresolved, store, flag, reference, nonparametered_action, parametered_action };

		enum class extra_capability : unsigned { Store = static_cast<unsigned>(v2_flag::Reference) + 1, Flag };

		constexpr auto bit(v2_flag flag) -> mask_type {
			return mask_type{1} << static_cast<unsigned>(flag);
		}

		constexpr auto bit(extra_capability capability) -> mask_type {
			return mask_type{1} << static_cast<unsigned>(capability);
		}

		constexpr mask_type short_argument = bit(v2_flag::ShortArgument);
		constexpr mask_type long_argument = bit(v2_flag::LongArgument);
		constexpr mask_type positional = bit(v2_flag::Positional);
		constexpr mask_type position = bit(v2_flag::Position);
		constexpr mask_type help_text = bit(v2_flag::HelpText);
		constexpr mask_type action = bit(v2_flag::Action);
		constexpr mask_type required = bit(v2_flag::Required);
		constexpr mask_type reference = bit(v2_flag::Reference);
		constexpr mask_type store = bit(extra_capability::Store);
		constexpr mask_type flag = bit(extra_capability::Flag);

		constexpr mask_type value_mode_group = action | reference | store | flag;
		constexpr mask_type initial =
			short_argument | long_argument | positional | help_text | action | required | reference | store | flag;

		constexpr auto has(mask_type mask, mask_type capability) -> bool {
			return (mask & capability) == capability;
		}

		constexpr auto remove(mask_type mask, mask_type capability) -> mask_type {
			return mask & ~capability;
		}

		constexpr auto replace(mask_type mask, mask_type remove_bits, mask_type add_bits = 0) -> mask_type {
			return (mask & ~remove_bits) | add_bits;
		}

		constexpr auto has_selected_identifier(mask_type mask) -> bool {
			return !has(mask, short_argument) || !has(mask, long_argument) || !has(mask, positional);
		}

		constexpr auto is_buildable(mask_type mask) -> bool {
			return has_selected_identifier(mask);
		}
	} // namespace builder_mask

	template <builder_mask::mask_type mask = builder_mask::initial, typename store_type = non_type> class argument {
	public:
		using mask_type = builder_mask::mask_type;
		using v2_flag = argument_parser::v2::add_argument_flags;
		using value_mode = builder_mask::value_mode;

		static auto start() -> argument<builder_mask::initial> {
			return {};
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::short_argument), int> = 0>
		auto short_argument(std::string short_name) const
			-> argument<builder_mask::replace(current_mask, builder_mask::short_argument | builder_mask::positional |
																builder_mask::position),
						store_type> {
			using next_argument =
				argument<builder_mask::replace(current_mask, builder_mask::short_argument | builder_mask::positional |
																 builder_mask::position),
						 store_type>;

			next_argument next{*this};
			next.m_short_argument = std::move(short_name);
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::long_argument), int> = 0>
		auto long_argument(std::string long_name) const
			-> argument<builder_mask::replace(current_mask, builder_mask::long_argument | builder_mask::positional |
																builder_mask::position),
						store_type> {
			using next_argument =
				argument<builder_mask::replace(current_mask, builder_mask::long_argument | builder_mask::positional |
																 builder_mask::position),
						 store_type>;

			next_argument next{*this};
			next.m_long_argument = std::move(long_name);
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::positional), int> = 0>
		auto positional(std::string positional_name) const
			-> argument<builder_mask::replace(current_mask,
											  builder_mask::short_argument | builder_mask::long_argument |
												  builder_mask::positional | builder_mask::flag,
											  builder_mask::position),
						store_type> {
			using next_argument =
				argument<builder_mask::replace(current_mask,
											   builder_mask::short_argument | builder_mask::long_argument |
												   builder_mask::positional | builder_mask::flag,
											   builder_mask::position),
						 store_type>;

			next_argument next{*this};
			next.m_positional_name = std::move(positional_name);
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::position), int> = 0>
		auto position(int index) const
			-> argument<builder_mask::remove(current_mask, builder_mask::position), store_type> {
			using next_argument = argument<builder_mask::remove(current_mask, builder_mask::position), store_type>;

			next_argument next{*this};
			next.m_position = index;
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::help_text), int> = 0>
		auto help_text(std::string help) const
			-> argument<builder_mask::remove(current_mask, builder_mask::help_text), store_type> {
			using next_argument = argument<builder_mask::remove(current_mask, builder_mask::help_text), store_type>;

			next_argument next{*this};
			next.m_help_text = std::move(help);
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::required), int> = 0>
		auto required(bool value = true) const
			-> argument<builder_mask::remove(current_mask, builder_mask::required), store_type> {
			using next_argument = argument<builder_mask::remove(current_mask, builder_mask::required), store_type>;

			next_argument next{*this};
			next.m_required = value;
			return next;
		}

		template <typename T = std::string, mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::store), int> = 0>
		auto store() const -> argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), T> {
			static_assert(!std::is_same_v<T, void>,
						  "store<void>() is not supported. Use flag() for boolean-style arguments.");

			using next_argument = argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), T>;
			next_argument next{*this};
			next.m_value_mode = value_mode::store;
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::flag), int> = 0>
		auto flag() const -> argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), bool> {
			using next_argument = argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), bool>;

			next_argument next{*this};
			next.m_value_mode = value_mode::flag;
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::reference), int> = 0, typename T>
		auto reference(T &value) const
			-> argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), T> {
			using next_argument = argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), T>;

			next_argument next{*this};
			next.m_reference = std::addressof(value);
			next.m_value_mode = value_mode::reference;
			return next;
		}

		template <mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::action), int> = 0, typename Callable>
		auto action(Callable &&handler) const -> std::enable_if_t<
			std::is_invocable_r_v<void, Callable>,
			argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), non_type>> {
			using next_argument =
				argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), non_type>;

			next_argument next{*this};
			next.m_action = std::make_shared<argument_parser::non_parametered_action>(
				std::function<void()>(std::forward<Callable>(handler)));
			next.m_value_mode = value_mode::nonparametered_action;
			return next;
		}

		template <typename T = std::string, mask_type current_mask = mask,
				  std::enable_if_t<builder_mask::has(current_mask, builder_mask::action), int> = 0, typename Callable>
		auto action(Callable &&handler) const
			-> std::enable_if_t<std::is_invocable_r_v<void, Callable, const T &>,
								argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), T>> {
			static_assert(!std::is_same_v<T, void>,
						  "action<void>(...) is not supported. Use action([] { ... }) instead.");

			using next_argument = argument<builder_mask::remove(current_mask, builder_mask::value_mode_group), T>;

			next_argument next{*this};
			next.m_action = std::make_shared<argument_parser::parametered_action<T>>(
				std::function<void(const T &)>(std::forward<Callable>(handler)));
			next.m_value_mode = value_mode::parametered_action;
			return next;
		}

		template <mask_type current_mask = mask, std::enable_if_t<builder_mask::is_buildable(current_mask), int> = 0>
		auto build(argument_parser::v2::base_parser &parser) const -> void {
			assert_has_identifier();

			switch (m_value_mode) {
			case value_mode::flag:
				build_flag(parser);
				return;
			case value_mode::nonparametered_action:
				build_nonparametered_action(parser);
				return;
			case value_mode::store:
				if constexpr (!std::is_same_v<store_type, non_type>) {
					build_store(parser);
					return;
				}
				break;
			case value_mode::reference:
				if constexpr (!std::is_same_v<store_type, non_type>) {
					build_reference(parser);
					return;
				}
				break;
			case value_mode::parametered_action:
				if constexpr (!std::is_same_v<store_type, non_type>) {
					build_parametered_action(parser);
					return;
				}
				break;
			case value_mode::unresolved:
				if (is_positional()) {
					build_default_positional(parser);
				} else {
					build_flag(parser);
				}
				return;
			}

			throw std::logic_error("The builder reached build() without a supported terminal value mode.");
		}

	private:
		argument() = default;

		template <mask_type other_mask, typename other_store_type>
		argument(argument<other_mask, other_store_type> const &other)
			: m_short_argument(other.m_short_argument), m_long_argument(other.m_long_argument),
			  m_positional_name(other.m_positional_name), m_position(other.m_position), m_help_text(other.m_help_text),
			  m_required(other.m_required), m_action(other.m_action), m_reference(copy_reference(other.m_reference)),
			  m_value_mode(other.m_value_mode) {}

		template <typename T>
		using typed_map =
			std::unordered_map<v2_flag, typename argument_parser::v2::base_parser::template typed_flag_value<T>>;

		using non_typed_map = std::unordered_map<v2_flag, argument_parser::v2::base_parser::non_typed_flag_value>;

		auto is_positional() const -> bool {
			return !m_positional_name.empty();
		}

		auto lookup_key() const -> std::string {
			if (is_positional()) {
				return m_positional_name;
			}
			if (!m_long_argument.empty()) {
				return m_long_argument;
			}
			if (!m_short_argument.empty()) {
				return m_short_argument;
			}

			throw std::logic_error("No argument identifier is available for lookup.");
		}

		auto assert_has_identifier() const -> void {
			if (!is_positional() && m_short_argument.empty() && m_long_argument.empty()) {
				throw std::logic_error("build() requires short_argument(), long_argument(), or positional().");
			}
		}

		template <typename Map> auto add_common_pairs(Map &pairs) const -> void {
			using namespace argument_parser::v2::flags;

			if (is_positional()) {
				pairs[Positional] = m_positional_name;
				if (m_position.has_value()) {
					pairs[Position] = m_position.value();
				}
			} else {
				if (!m_short_argument.empty()) {
					pairs[ShortArgument] = m_short_argument;
				}
				if (!m_long_argument.empty()) {
					pairs[LongArgument] = m_long_argument;
				}
			}

			if (!m_help_text.empty()) {
				pairs[HelpText] = m_help_text;
			}
			if (m_required) {
				pairs[Required] = true;
			}
		}

		template <typename T> auto make_typed_pairs() const -> typed_map<T> {
			typed_map<T> pairs;
			add_common_pairs(pairs);
			return pairs;
		}

		auto make_non_typed_pairs() const -> non_typed_map {
			non_typed_map pairs;
			add_common_pairs(pairs);
			return pairs;
		}

		auto build_flag(argument_parser::v2::base_parser &parser) const -> void {
			auto pairs = make_non_typed_pairs();
			parser.add_argument(pairs);
		}

		auto build_default_positional(argument_parser::v2::base_parser &parser) const -> void {
			auto pairs = make_non_typed_pairs();
			parser.add_argument(pairs);
		}

		auto build_store(argument_parser::v2::base_parser &parser) const -> void {
			auto pairs = make_typed_pairs<store_type>();
			parser.template add_argument<store_type>(pairs);
		}

		auto build_reference(argument_parser::v2::base_parser &parser) const -> void {
			auto pairs = make_typed_pairs<store_type>();
			auto *target = m_reference;
			auto key = lookup_key();

			if (target == nullptr) {
				throw std::logic_error("reference() was selected without a target.");
			}

			pairs[argument_parser::v2::flags::Reference] = target;
			parser.template add_argument<store_type>(pairs);
		}

		auto build_parametered_action(argument_parser::v2::base_parser &parser) const -> void {
			auto const *typed_action =
				dynamic_cast<argument_parser::parametered_action<store_type> const *>(m_action.get());
			if (typed_action == nullptr) {
				throw std::logic_error("Stored action is not compatible with the requested parameter type.");
			}

			auto pairs = make_typed_pairs<store_type>();
			pairs[argument_parser::v2::flags::Action] = *typed_action;
			parser.template add_argument<store_type>(pairs);
		}

		auto build_nonparametered_action(argument_parser::v2::base_parser &parser) const -> void {
			auto const *nonparametered_action =
				dynamic_cast<argument_parser::non_parametered_action const *>(m_action.get());
			if (nonparametered_action == nullptr) {
				throw std::logic_error("Stored action is not a non-parametered action.");
			}

			if (is_positional()) {
				auto wrapped_action = argument_parser::helpers::make_parametered_action<std::string>(
					[action = *nonparametered_action](std::string const &) { action.invoke(); });
				auto pairs = make_typed_pairs<std::string>();
				pairs[argument_parser::v2::flags::Action] = wrapped_action;
				parser.template add_argument<std::string>(pairs);
				return;
			}

			auto pairs = make_non_typed_pairs();
			pairs[argument_parser::v2::flags::Action] = *nonparametered_action;
			parser.add_argument(pairs);
		}

		std::string m_short_argument{};
		std::string m_long_argument{};
		std::string m_positional_name{};
		std::optional<int> m_position{};
		std::string m_help_text{};
		bool m_required = false;
		std::shared_ptr<argument_parser::action_base const> m_action{};
		store_type *m_reference = nullptr;
		value_mode m_value_mode = value_mode::unresolved;

		template <typename other_store_type> static auto copy_reference(other_store_type *reference) -> store_type * {
			if constexpr (std::is_same_v<store_type, other_store_type>) {
				return reference;
			} else {
				return nullptr;
			}
		}

		template <mask_type other_mask, typename other_store_type> friend class argument;
	};

	namespace assertions {
		struct noop_handler {
			void operator()() const {}
		};

		template <typename T> struct parameter_sink {
			void operator()(T const &) const {}
		};

		template <typename T, typename = void> struct can_use_help_text : std::false_type {};

		template <typename T>
		struct can_use_help_text<T, std::void_t<decltype(std::declval<T>().help_text(std::declval<std::string>()))>>
			: std::true_type {};

		template <typename T, typename = void> struct can_use_position : std::false_type {};

		template <typename T>
		struct can_use_position<T, std::void_t<decltype(std::declval<T>().position(0))>> : std::true_type {};

		template <typename T, typename = void> struct can_use_nonparametered_action : std::false_type {};

		template <typename T>
		struct can_use_nonparametered_action<T, std::void_t<decltype(std::declval<T>().action(noop_handler{}))>>
			: std::true_type {};

		template <typename T, typename U, typename = void> struct can_use_parametered_action : std::false_type {};

		template <typename T, typename U>
		struct can_use_parametered_action<
			T, U, std::void_t<decltype(std::declval<T>().template action<U>(parameter_sink<U>{}))>> : std::true_type {};

		template <typename T, typename U, typename = void> struct can_use_store : std::false_type {};

		template <typename T, typename U>
		struct can_use_store<T, U, std::void_t<decltype(std::declval<T>().template store<U>())>> : std::true_type {};

		template <typename T, typename = void> struct can_use_flag : std::false_type {};

		template <typename T>
		struct can_use_flag<T, std::void_t<decltype(std::declval<T>().flag())>> : std::true_type {};

		template <typename T, typename U, typename = void> struct can_use_reference : std::false_type {};

		template <typename T, typename U>
		struct can_use_reference<T, U, std::void_t<decltype(std::declval<T>().reference(std::declval<U &>()))>>
			: std::true_type {};

		using after_help_text = decltype(argument<>::start().help_text("help"));
		static_assert(!can_use_help_text<after_help_text>::value, "help_text() should be single-use.");

		using after_positional = decltype(argument<>::start().positional("path"));
		static_assert(can_use_position<after_positional>::value, "positional() should unlock position().");

		using after_position = decltype(argument<>::start().positional("path").position(0));
		static_assert(!can_use_position<after_position>::value, "position() should be single-use.");

		using after_positional_mode_selection = decltype(argument<>::start().positional("path").store<>());
		static_assert(!can_use_flag<after_positional_mode_selection>::value,
					  "flag() should not be available for positional arguments.");

		using after_nonparametered_action =
			decltype(argument<>::start().short_argument("v").help_text("verbose").action(noop_handler{}));
		static_assert(!can_use_nonparametered_action<after_nonparametered_action>::value,
					  "action() should not remain callable after selecting a non-parametered action.");
		static_assert(!can_use_parametered_action<after_nonparametered_action, int>::value,
					  "typed action() should also be disabled after selecting a non-parametered action.");
		static_assert(!can_use_store<after_nonparametered_action, int>::value,
					  "store() should be mutually exclusive with action().");
		static_assert(!can_use_flag<after_nonparametered_action>::value,
					  "flag() should be mutually exclusive with action().");
		static_assert(!can_use_reference<after_nonparametered_action, int>::value,
					  "reference() should be mutually exclusive with action().");

		using after_parametered_action =
			decltype(argument<>::start().positional("count").position(0).action<int>(parameter_sink<int>{}));
		static_assert(!can_use_nonparametered_action<after_parametered_action>::value,
					  "non-parametered action() should be disabled after selecting a typed action.");
		static_assert(!can_use_parametered_action<after_parametered_action, std::string>::value,
					  "typed action() should be single-use regardless of the parameter type.");

	} // namespace assertions
} // namespace argument_parser::builder

#endif
