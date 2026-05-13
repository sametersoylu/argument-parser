#pragma once
#include "traits.hpp"
#include <argument_parser.hpp>
#include <array>
#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace argument_parser::v2 {
	enum class add_argument_flags {
		ShortArgument,
		LongArgument,
		Positional,
		Position,
		HelpText,
		Action,
		Required,
		Reference,
		Accumulate
	};

	namespace flags {
		constexpr static inline add_argument_flags ShortArgument = add_argument_flags::ShortArgument;
		constexpr static inline add_argument_flags LongArgument = add_argument_flags::LongArgument;
		constexpr static inline add_argument_flags HelpText = add_argument_flags::HelpText;
		constexpr static inline add_argument_flags Action = add_argument_flags::Action;
		constexpr static inline add_argument_flags Required = add_argument_flags::Required;
		constexpr static inline add_argument_flags Positional = add_argument_flags::Positional;
		constexpr static inline add_argument_flags Position = add_argument_flags::Position;
		constexpr static inline add_argument_flags Reference = add_argument_flags::Reference;
		constexpr static inline add_argument_flags Accumulate = add_argument_flags::Accumulate;
	} // namespace flags

	namespace deducers {
		template <typename, typename = void> struct has_value_type : std::false_type {};
		template <typename T> struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};

		template <typename T> struct is_vector {
			static constexpr bool test() {
				if constexpr (has_value_type<T>::value) {
					return std::is_same_v<T, std::vector<typename T::value_type, typename T::allocator_type>>;
				} else {
					return false;
				}
			}
		};

		template <typename T> constexpr bool is_vector_v = is_vector<T>::test();
	} // namespace deducers

	class base_parser : argument_parser::base_parser {
	public:
		template <typename T> using typed_flag_value = std::variant<std::string, action_with_param<T>, bool, int, T *>;
		using non_typed_flag_value = std::variant<std::string, action_no_param, bool, int>;

		template <typename T> using typed_argument_pair = std::pair<add_argument_flags, typed_flag_value<T>>;
		using non_typed_argument_pair = std::pair<add_argument_flags, non_typed_flag_value>;

		template <typename T>
		void add_argument(std::unordered_map<add_argument_flags, typed_flag_value<T>> const &argument_pairs) {
			add_argument_impl<true, action_with_param<T>, T>(argument_pairs);
		}

		template <typename T> void add_argument(std::initializer_list<typed_argument_pair<T>> const &pairs) {
			std::unordered_map<add_argument_flags, typed_flag_value<T>> args;

			for (auto &[k, v] : pairs) {
				args[k] = v;
			}

			add_argument<T>(args);
		}

		void add_argument(std::initializer_list<non_typed_argument_pair> const &pairs) {
			std::unordered_map<add_argument_flags, non_typed_flag_value> args;

			for (auto &[k, v] : pairs) {
				args[k] = v;
			}

			add_argument(args);
		}

		void add_argument(std::unordered_map<add_argument_flags, non_typed_flag_value> const &argument_pairs) {
			add_argument_impl<false, action_no_param, void>(argument_pairs);
		}

		argument_parser::base_parser &to_v1() {
			return *this;
		}

		void handle_arguments(std::initializer_list<conventions::convention const *const> convention_types) {
			base::handle_arguments(convention_types);
		}

		template <typename T> std::optional<T> get_optional(std::string const &arg) {
			return base::get_optional<T>(arg);
		}

		using argument_parser::base_parser::display_help;
		using argument_parser::base_parser::on_complete;

	protected:
		void set_program_name(std::string p) {
			base::program_name = std::move(p);
		}

		std::vector<std::string> &ref_parsed_args() {
			return base::parsed_arguments;
		}

		using argument_parser::base_parser::current_conventions;
		using argument_parser::base_parser::reset_current_conventions;

		void prepare_help_flag(bool should_exit = true) {
			add_argument({{flags::ShortArgument, "h"},
						  {flags::LongArgument, "help"},
						  {flags::Action, helpers::make_action([this, should_exit] {
							   this->display_help(this->current_conventions());
							   if (should_exit) {
								   std::exit(0);
							   }
						   })},
						  {flags::HelpText, "Prints this help text."}});
		}

		void set_settings(parser_settings const &settings) {
			base::set_settings(settings);
		}

	private:
		template <bool IsTyped, typename ActionType, typename T, typename ArgsMap>
		void add_argument_impl(ArgsMap const &argument_pairs) {
			if (argument_pairs.find(add_argument_flags::Positional) != argument_pairs.end()) {
				add_positional_argument_impl<IsTyped, ActionType, T>(argument_pairs);
				return;
			}

			std::unordered_map<extended_add_argument_flags, bool> found_params{
				{extended_add_argument_flags::IsTyped, IsTyped}};

			std::string short_arg, long_arg, help_text;
			std::unique_ptr<action_base> action;
			bool required = false;
			bool accumulates = false;

			if (has_flag(argument_pairs, add_argument_flags::ShortArgument)) {
				found_params[extended_add_argument_flags::ShortArgument] = true;
				short_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::ShortArgument), "short");
			}
			if (has_flag(argument_pairs, add_argument_flags::LongArgument)) {
				found_params[extended_add_argument_flags::LongArgument] = true;
				long_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::LongArgument), "long");
				if (short_arg.empty())
					short_arg = "-";
			} else {
				if (!short_arg.empty())
					long_arg = "-";
			}

			if (has_flag(argument_pairs, add_argument_flags::Action)) {
				found_params[extended_add_argument_flags::Action] = true;
				action = get_or_throw<ActionType>(argument_pairs.at(add_argument_flags::Action), "action").clone();
			}
			help_text = read_help_text(argument_pairs);
			required = read_required(argument_pairs);

			bool ref_mode = false;

			if (has_flag(argument_pairs, add_argument_flags::Reference)) {
				ref_mode = true;
				if (!IsTyped) {
					throw std::logic_error("Reference argument must be typed");
				}

				found_params[extended_add_argument_flags::Action] = true;
				if constexpr (!std::is_same_v<T, void>) {
					auto ref = get_or_throw<T *>(argument_pairs.at(add_argument_flags::Reference), "reference");
					if (action) {
						throw std::logic_error("Cannot use both action and reference for the same argument");
					}
					action = make_reference_action(ref);
				} else {
					throw std::logic_error("Reference argument must not be void");
				}
			}

			if (has_flag(argument_pairs, add_argument_flags::Accumulate)) {
				if (!IsTyped)
					throw std::logic_error("Accumulate argument must be typed");

				found_params[extended_add_argument_flags::Action] = true;
				accumulates = true;
				if constexpr (!std::is_same_v<T, void>) {
					if constexpr (std::is_same_v<T, int>) {
						action = make_accumulate_action<T>(argument_pairs, ref_mode, short_arg, long_arg);
					} else if constexpr (!deducers::is_vector_v<T>) {
						throw std::logic_error("Expected vector or integer type");
					} else {
						if (action && !ref_mode) {
							throw std::logic_error("Cannot use both action and accumulate for the same argument");
						}

						action = make_accumulate_action<T>(argument_pairs, ref_mode, short_arg, long_arg);
					}
				} else {
					throw std::logic_error("Accumulate argument must not be void");
				}
			}

			if (accumulates) {
				if constexpr (!std::is_same_v<T, void> && deducers::is_vector_v<T>) {
					if (suggest_candidate(found_params) == candidate_type::unknown) {
						throw std::runtime_error(
							"Could not match any add argument overload to given parameters. Are you "
							"missing some required parameter?");
					}
					if (help_text.empty()) {
						help_text = "Accepts repeated values.";
					}

					base::add_argument<typename T::value_type>(
						short_arg, long_arg, help_text,
						*static_cast<action_with_param<typename T::value_type> *>(&(*action)), required);
					return;
				}
			}

			auto suggested_add = suggest_candidate(found_params);
			if (suggested_add == candidate_type::unknown) {
				throw std::runtime_error("Could not match any add argument overload to given parameters. Are you "
										 "missing some required parameter?");
			}

			if constexpr (IsTyped) {
				switch (suggested_add) {
				case candidate_type::typed_action:
					if (help_text.empty()) {
						if constexpr (internal::sfinae::has_format_hint<parsing_traits::parser_trait<T>>::value &&
									  internal::sfinae::has_purpose_hint<parsing_traits::parser_trait<T>>::value) {
							auto format_hint = parsing_traits::parser_trait<T>::format_hint;
							auto purpose_hint = parsing_traits::parser_trait<T>::purpose_hint;
							help_text = "Triggers action with " + std::string(purpose_hint) + " (" +
										std::string(format_hint) + ")";
						} else {
							help_text = "Triggers action with value.";
						}
					}

					base::add_argument(short_arg, long_arg, help_text, *static_cast<ActionType *>(&(*action)),
									   required);
					break;
				case candidate_type::store_other:
					if (help_text.empty()) {
						if constexpr (internal::sfinae::has_format_hint<parsing_traits::parser_trait<T>>::value &&
									  internal::sfinae::has_purpose_hint<parsing_traits::parser_trait<T>>::value) {
							auto format_hint = parsing_traits::parser_trait<T>::format_hint;
							auto purpose_hint = parsing_traits::parser_trait<T>::purpose_hint;
							help_text =
								"Accepts " + std::string(purpose_hint) + " in " + std::string(format_hint) + " format.";
						} else {
							help_text = "Accepts value.";
						}
					}

					base::add_argument<T>(short_arg, long_arg, help_text, required);
					break;
				default:
					throw std::runtime_error("Could not match the arguments against any overload.");
				}
			} else {
				switch (suggested_add) {
				case candidate_type::non_typed_action:
					if (help_text.empty()) {
						help_text = "Triggers action with no value.";
					}

					base::add_argument(short_arg, long_arg, help_text, *static_cast<ActionType *>(&(*action)),
									   required);
					break;
				case candidate_type::store_boolean:
					if (help_text.empty()) {
						auto boolPurpose = parsing_traits::parser_trait<bool>::purpose_hint;
						auto boolFormat = parsing_traits::parser_trait<bool>::format_hint;
						help_text =
							"Accepts " + std::string(boolPurpose) + " in " + std::string(boolFormat) + " format.";
					}

					base::add_argument(short_arg, long_arg, help_text, required);
					break;
				default:
					throw std::runtime_error(
						"Could not match the arguments against any overload. The suggested candidate was: " +
						std::to_string(static_cast<int>(suggested_add)));
				}
			}
		}

		template <bool IsTyped, typename ActionType, typename T, typename ArgsMap>
		void add_positional_argument_impl(ArgsMap const &argument_pairs) {
			auto positional_name =
				get_or_throw<std::string>(argument_pairs.at(add_argument_flags::Positional), "positional");

			std::unique_ptr<action_base> action;
			bool required = false;
			bool ref_mode = false;
			bool accumulates = false;

			if (has_flag(argument_pairs, add_argument_flags::Action)) {
				action = get_or_throw<ActionType>(argument_pairs.at(add_argument_flags::Action), "action").clone();
			}
			std::string help_text = read_help_text(argument_pairs);
			required = read_required(argument_pairs);
			std::optional<int> position = read_position(argument_pairs);

			if (has_flag(argument_pairs, add_argument_flags::Reference)) {
				ref_mode = true;
				if (!IsTyped) {
					throw std::logic_error("Reference argument must be typed");
				}

				if constexpr (!std::is_same_v<T, void>) {
					if (!has_flag(argument_pairs, add_argument_flags::Accumulate)) {
						auto ref = get_or_throw<T *>(argument_pairs.at(add_argument_flags::Reference), "reference");
						if (action) {
							throw std::logic_error("Cannot use both action and reference for the same argument");
						}

						action = make_reference_action(ref);
					}
				} else {
					throw std::logic_error("Reference argument must not be void");
				}
			}

			if (has_flag(argument_pairs, add_argument_flags::Accumulate)) {
				if (!IsTyped)
					throw std::logic_error("Accumulate positional argument must be typed");

				accumulates = true;
				if constexpr (!std::is_same_v<T, void>) {
					if constexpr (!deducers::is_vector_v<T>) {
						throw std::logic_error("Expected vector (type does not have value_type member)");
					} else {
						if (action && !ref_mode) {
							throw std::logic_error("Cannot use both action and accumulate for the same argument");
						}
						action = make_accumulate_action<T>(argument_pairs, ref_mode, positional_name);
					}
				} else {
					throw std::logic_error("Accumulate argument must not be void");
				}
			}

			if (help_text.empty()) {
				if constexpr (IsTyped) {
					if constexpr (internal::sfinae::has_format_hint<parsing_traits::parser_trait<T>>::value &&
								  internal::sfinae::has_purpose_hint<parsing_traits::parser_trait<T>>::value) {
						auto format_hint = parsing_traits::parser_trait<T>::format_hint;
						auto purpose_hint = parsing_traits::parser_trait<T>::purpose_hint;
						help_text =
							"Accepts " + std::string(purpose_hint) + " in " + std::string(format_hint) + " format.";
					} else {
						help_text = "Accepts value.";
					}
				} else {
					help_text = "Accepts value.";
				}
			}

			if (accumulates) {
				if constexpr (!std::is_same_v<T, void> && deducers::is_vector_v<T>) {
					base::add_positional_accumulator<typename T::value_type>(
						positional_name, help_text,
						*static_cast<action_with_param<typename T::value_type> *>(&(*action)), required, position);
					return;
				}
			}

			if constexpr (IsTyped) {
				if (action) {
					base::add_positional_argument<T>(positional_name, help_text, *static_cast<ActionType *>(&(*action)),
													 required, position);
				} else {
					base::add_positional_argument<T>(positional_name, help_text, required, position);
				}
			} else {
				base::add_positional_argument<std::string>(positional_name, help_text, required, position);
			}
		}

		using base = argument_parser::base_parser;
		enum class extended_add_argument_flags { ShortArgument, LongArgument, Action, IsTyped };

		enum class candidate_type { store_boolean, store_other, typed_action, non_typed_action, unknown };

		template <typename T, size_t S>
		bool satisfies_at_least_one(std::array<T, S> const &arr, std::unordered_map<T, bool> const &map) {
			return std::any_of(arr.begin(), arr.end(), [&map](T const &entry) { return map.find(entry) != map.end(); });
		}

		candidate_type suggest_candidate(std::unordered_map<extended_add_argument_flags, bool> const &available_vars) {
			auto constexpr required_at_least_one = std::array<extended_add_argument_flags, 2>{
				extended_add_argument_flags::ShortArgument, extended_add_argument_flags::LongArgument};
			if (!satisfies_at_least_one(required_at_least_one, available_vars))
				return candidate_type::unknown;

			if (available_vars.find(extended_add_argument_flags::Action) != available_vars.end()) {
				if (available_vars.at(extended_add_argument_flags::IsTyped))
					return candidate_type::typed_action;
				return candidate_type::non_typed_action;
			}

			if (available_vars.at(extended_add_argument_flags::IsTyped))
				return candidate_type::store_other;
			return candidate_type::store_boolean;
		}

		template <typename ArgsMap> static bool has_flag(ArgsMap const &argument_pairs, add_argument_flags flag) {
			return argument_pairs.find(flag) != argument_pairs.end();
		}

		template <typename ArgsMap> std::string read_help_text(ArgsMap const &argument_pairs) {
			if (has_flag(argument_pairs, add_argument_flags::HelpText)) {
				return get_or_throw<std::string>(argument_pairs.at(add_argument_flags::HelpText), "help");
			}
			return "";
		}

		template <typename ArgsMap> bool read_required(ArgsMap const &argument_pairs) {
			return has_flag(argument_pairs, add_argument_flags::Required) &&
				   get_or_throw<bool>(argument_pairs.at(add_argument_flags::Required), "required");
		}

		template <typename ArgsMap> std::optional<int> read_position(ArgsMap const &argument_pairs) {
			if (has_flag(argument_pairs, add_argument_flags::Position)) {
				return get_or_throw<int>(argument_pairs.at(add_argument_flags::Position), "position");
			}
			return std::nullopt;
		}

		template <typename T, typename T2, typename I>
		std::variant<T, T2> get_either_or_throw(typed_flag_value<I> const &v, std::string_view key) {
			if (auto p = std::get_if<T>(&v))
				return *p;
			if (auto p = std::get_if<T2>(&v))
				return *p;
			throw std::invalid_argument(std::string("variant type mismatch for key: ") + std::string(key));
		}

		template <typename T> std::unique_ptr<action_base> make_reference_action(T *target) {
			return helpers::make_action<T>([target](T const &value) { *target = value; }).clone();
		}

		template <typename Vector> std::unique_ptr<action_base> make_accumulate_ref_action(Vector *target) {
			if constexpr (std::is_same_v<Vector, int>) {
				return helpers::make_action([target]() { *target += 1; }).clone();
			} else {
				using Value = typename Vector::value_type;
				return helpers::make_action<Value>([target](Value const &value) { target->emplace_back(value); })
					.clone();
			}
		}

		template <typename Vector>
		void store_accumulated_on_complete(std::string short_arg, std::string long_arg,
										   std::shared_ptr<Vector> accumulation_target) {
			on_complete(
				[this, short_arg = std::move(short_arg), long_arg = std::move(long_arg),
				 accumulation_target](auto const &) {
					if constexpr (std::is_same_v<int, Vector>) {
						if (*accumulation_target == 0) {
							return;
						}
					} else {
						if (accumulation_target->empty()) {
							return;
						}
					}

					const auto sid = this->find_argument_id(short_arg);
					const auto lid = this->find_argument_id(long_arg);

					if (const auto id = sid ? *sid : (lid ? *lid : -1); id != -1) {
						this->ref_stored_arguments()[id] = *accumulation_target;
					}
				},
				true);
		}

		template <typename Vector>
		void store_accumulated_on_complete(std::string positional_name, std::shared_ptr<Vector> accumulation_target) {
			on_complete(
				[this, positional_name = std::move(positional_name), accumulation_target](auto const &) {
					if constexpr (std::is_same_v<int, Vector>) {
						if (*accumulation_target == 0) {
							return;
						}
					} else {
						if (accumulation_target->empty()) {
							return;
						}
					}

					auto id = this->find_argument_id(positional_name);
					if (id.has_value()) {
						this->ref_stored_arguments()[*id] = *accumulation_target;
					}
				},
				true);
		}

		template <typename Vector, typename ArgsMap>
		std::unique_ptr<action_base> make_accumulate_action(ArgsMap const &argument_pairs, bool ref_mode,
															std::string const &short_arg, std::string const &long_arg) {
			if (ref_mode) {
				auto ref = get_or_throw<Vector *>(argument_pairs.at(add_argument_flags::Reference), "reference");
				return make_accumulate_ref_action(ref);
			}

			auto accumulate =
				get_either_or_throw<Vector *, bool>(argument_pairs.at(add_argument_flags::Accumulate), "accumulate");

			return std::visit(
				[this, short_arg, long_arg](auto &&acc) -> std::unique_ptr<action_base> {
					using V = std::decay_t<decltype(acc)>;
					if constexpr (std::is_same_v<V, bool>) {
						if (!acc) {
							throw std::logic_error("Accumulate flag must be true when used as a bool");
						}

						auto accumulation_target = std::make_shared<Vector>();
						store_accumulated_on_complete(short_arg, long_arg, accumulation_target);
						return make_accumulate_ref_action(accumulation_target.get());
					} else {
						return make_accumulate_ref_action(acc);
					}
				},
				accumulate);
		}

		template <typename Vector, typename ArgsMap>
		std::unique_ptr<action_base> make_accumulate_action(ArgsMap const &argument_pairs, bool ref_mode,
															std::string const &positional_name) {
			if (ref_mode) {
				auto ref = get_or_throw<Vector *>(argument_pairs.at(add_argument_flags::Reference), "reference");
				return make_accumulate_ref_action(ref);
			}

			auto accumulate =
				get_either_or_throw<Vector *, bool>(argument_pairs.at(add_argument_flags::Accumulate), "accumulate");

			return std::visit(
				[this, positional_name](auto &&acc) -> std::unique_ptr<action_base> {
					using V = std::decay_t<decltype(acc)>;
					if constexpr (std::is_same_v<V, bool>) {
						if (!acc) {
							throw std::logic_error("Accumulate flag must be true when used as a bool");
						}

						auto accumulation_target = std::make_shared<Vector>();
						store_accumulated_on_complete(positional_name, accumulation_target);
						return make_accumulate_ref_action(accumulation_target.get());
					} else {
						return make_accumulate_ref_action(acc);
					}
				},
				accumulate);
		}

		template <typename T, typename I> T get_or_throw(typed_flag_value<I> const &v, std::string_view key) {
			if (auto p = std::get_if<T>(&v))
				return *p;
			throw std::invalid_argument(std::string("variant type mismatch for key: ") + std::string(key));
		}

		template <typename T> T get_or_throw(non_typed_flag_value const &v, std::string_view key) {
			if (auto p = std::get_if<T>(&v))
				return *p;
			throw std::invalid_argument(std::string("variant type mismatch for key: ") + std::string(key));
		}
	};
} // namespace argument_parser::v2
