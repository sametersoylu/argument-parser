#pragma once
#include "traits.hpp"
#include <argument_parser.hpp>
#include <array>
#include <cstdlib>
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
	enum class add_argument_flags { ShortArgument, LongArgument, Positional, Position, HelpText, Action, Required, Reference };

	namespace flags {
		constexpr static inline add_argument_flags ShortArgument = add_argument_flags::ShortArgument;
		constexpr static inline add_argument_flags LongArgument = add_argument_flags::LongArgument;
		constexpr static inline add_argument_flags HelpText = add_argument_flags::HelpText;
		constexpr static inline add_argument_flags Action = add_argument_flags::Action;
		constexpr static inline add_argument_flags Required = add_argument_flags::Required;
		constexpr static inline add_argument_flags Positional = add_argument_flags::Positional;
		constexpr static inline add_argument_flags Position = add_argument_flags::Position;
		constexpr static inline add_argument_flags Reference = add_argument_flags::Reference;
	} // namespace flags

	class base_parser : private argument_parser::base_parser {
	public:
		template <typename T> using typed_flag_value = std::variant<std::string, parametered_action<T>, bool, int, T*>;
		using non_typed_flag_value = std::variant<std::string, non_parametered_action, bool, int>;

		template <typename T> using typed_argument_pair = std::pair<add_argument_flags, typed_flag_value<T>>;
		using non_typed_argument_pair = std::pair<add_argument_flags, non_typed_flag_value>;

		template <typename T>
		void add_argument(std::unordered_map<add_argument_flags, typed_flag_value<T>> const &argument_pairs) {
			add_argument_impl<true, parametered_action<T>, T>(argument_pairs);
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
			add_argument_impl<false, non_parametered_action, void>(argument_pairs);
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
						  {flags::Action, helpers::make_non_parametered_action([this, should_exit]() {
							   this->display_help(this->current_conventions());
							   if (should_exit) {
								   std::exit(0);
							   }
						   })},
						  {flags::HelpText, "Prints this help text."}});
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

			if (argument_pairs.find(add_argument_flags::ShortArgument) != argument_pairs.end()) {
				found_params[extended_add_argument_flags::ShortArgument] = true;
				short_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::ShortArgument), "short");
			}
			if (argument_pairs.find(add_argument_flags::LongArgument) != argument_pairs.end()) {
				found_params[extended_add_argument_flags::LongArgument] = true;
				long_arg = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::LongArgument), "long");
				if (short_arg.empty())
					short_arg = "-";
			} else {
				if (!short_arg.empty())
					long_arg = "-";
			}

			if (argument_pairs.find(add_argument_flags::Action) != argument_pairs.end()) {
				found_params[extended_add_argument_flags::Action] = true;
				action = get_or_throw<ActionType>(argument_pairs.at(add_argument_flags::Action), "action").clone();
			}
			if (argument_pairs.find(add_argument_flags::HelpText) != argument_pairs.end()) {
				help_text = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::HelpText), "help");
			}

			if (argument_pairs.find(add_argument_flags::Required) != argument_pairs.end() &&
				get_or_throw<bool>(argument_pairs.at(add_argument_flags::Required), "required")) {
				required = true;
			}

			if (argument_pairs.find(add_argument_flags::Reference) != argument_pairs.end()) {
				if (!IsTyped) {
				    throw std::logic_error("Reference argument must be typed");
				}

				found_params[extended_add_argument_flags::Action] = true;
				if constexpr (!std::is_same_v<T, void>) {
				    auto ref = get_or_throw<T*>(argument_pairs.at(add_argument_flags::Reference), "reference");
					if (action) {
					    throw std::logic_error("Cannot use both action and reference for the same argument");
					} else {
						action = helpers::make_parametered_action<T>([ref](T const& t) {
							*ref = t;
						}).clone();
					}
				} else {
				    throw std::logic_error("Reference argument must not be void");
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

					base::add_argument(short_arg, long_arg, help_text, *static_cast<ActionType *>(&(*action)), required);
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
						std::to_string((int(suggested_add))));
				}
			}
		}

		template <bool IsTyped, typename ActionType, typename T, typename ArgsMap>
		void add_positional_argument_impl(ArgsMap const &argument_pairs) {
			std::string positional_name =
				get_or_throw<std::string>(argument_pairs.at(add_argument_flags::Positional), "positional");

			std::string help_text;
			std::unique_ptr<action_base> action;
			bool required = false;
			std::optional<int> position = std::nullopt;

			if (argument_pairs.find(add_argument_flags::Action) != argument_pairs.end()) {
				action = get_or_throw<ActionType>(argument_pairs.at(add_argument_flags::Action), "action").clone();
			}
			if (argument_pairs.find(add_argument_flags::HelpText) != argument_pairs.end()) {
				help_text = get_or_throw<std::string>(argument_pairs.at(add_argument_flags::HelpText), "help");
			}
			if (argument_pairs.find(add_argument_flags::Required) != argument_pairs.end() &&
				get_or_throw<bool>(argument_pairs.at(add_argument_flags::Required), "required")) {
				required = true;
			}
			if (argument_pairs.find(add_argument_flags::Position) != argument_pairs.end()) {
				position = get_or_throw<int>(argument_pairs.at(add_argument_flags::Position), "position");
			}

			if (argument_pairs.find(add_argument_flags::Reference) != argument_pairs.end()) {
				if (!IsTyped) {
				    throw std::logic_error("Reference argument must be typed");
				}

				if constexpr (!std::is_same_v<T, void>) {
				    auto ref = get_or_throw<T*>(argument_pairs.at(add_argument_flags::Reference), "reference");
					if (action) {
					    throw std::logic_error("Cannot use both action and reference for the same argument");
					} else {
						action = helpers::make_parametered_action<T>([ref](T const& t) {
							*ref = t;
						}).clone();
					}
				} else {
				    throw std::logic_error("Reference argument must not be void");
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

			if constexpr (IsTyped) {
				if (action) {
					base::add_positional_argument<T>(positional_name, help_text,
													 *static_cast<ActionType *>(&(*action)), required, position);
				} else {
					base::template add_positional_argument<T>(positional_name, help_text, required, position);
				}
			} else {
				base::template add_positional_argument<std::string>(positional_name, help_text, required, position);
			}
		}

		using base = argument_parser::base_parser;
		enum class extended_add_argument_flags { ShortArgument, LongArgument, Action, IsTyped };

		enum class candidate_type { store_boolean, store_other, typed_action, non_typed_action, unknown };

		template <typename T, size_t S>
		bool satisfies_at_least_one(std::array<T, S> const &arr, std::unordered_map<T, bool> const &map) {
			for (const auto &req : arr) {
				if (map.find(req) != map.end())
					return true;
			}
			return false;
		}

		candidate_type suggest_candidate(std::unordered_map<extended_add_argument_flags, bool> const &available_vars) {
			auto constexpr required_at_least_one = std::array<extended_add_argument_flags, 2>{
				extended_add_argument_flags::ShortArgument, extended_add_argument_flags::LongArgument};
			if (!satisfies_at_least_one(required_at_least_one, available_vars))
				return candidate_type::unknown;

			if (available_vars.find(extended_add_argument_flags::Action) != available_vars.end()) {
				if (available_vars.at(extended_add_argument_flags::IsTyped))
					return candidate_type::typed_action;
				else
					return candidate_type::non_typed_action;
			}

			if (available_vars.at(extended_add_argument_flags::IsTyped))
				return candidate_type::store_other;
			return candidate_type::store_boolean;
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
