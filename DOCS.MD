# Argument Parser API Documentation

This document covers the public API exposed by:

- `src/headers/parser/argument_parser.hpp`: the original v1 parser surface.
- `src/headers/parser/parser_v2.hpp`: the v2 map/initializer-list facade.
- `src/headers/parser/argument_builder.hpp`: the staged fluent builder for v2.

The lower-level v1 API still powers the platform parsers and v2. New code will
usually be clearest with `argument_parser::v2` and
`argument_parser::builder::new_argument()`, but the v1 types remain public and
useful when you need direct control over actions, stored values, or positional
registration.

## Common Concepts

Argument names are registered without command-line prefixes. Use `"v"` and
`"verbose"`, not `"-v"` or `"--verbose"`. The active convention objects decide
which concrete syntaxes are accepted during parsing and how help text is
rendered.

Typed values are parsed through
`argument_parser::parsing_traits::parser_trait<T>::parse(std::string const&)`.
The built-in traits cover common scalar types such as `std::string`, `bool`,
`int`, `float`, and `double`; custom value types need a trait specialization.
Trait `format_hint` and `purpose_hint` members are used in help text and parse
error messages when available.

Stored values are type-erased internally. Retrieve them with the same type used
when the argument was registered.

## v1 API: `argument_parser.hpp`

The public v1 types live in the `argument_parser` namespace unless otherwise
noted.

### `action_base`

```cpp
class action_base {
public:
    virtual ~action_base() = default;
    [[nodiscard]] virtual bool expects_parameter() const = 0;
    virtual void invoke() const = 0;
    virtual void invoke_with_parameter(std::string const& param) const = 0;
    [[nodiscard]] virtual std::pair<std::string, std::string> get_trait_hints() const = 0;
    [[nodiscard]] virtual std::unique_ptr<action_base> clone() const = 0;
};
```

`action_base` is the polymorphic interface stored by registered arguments.
Application code normally uses `action_with_param<T>`, `action_no_param`, or
the overloaded `helpers::make_action(...)` factories instead of implementing
this interface directly.

- `expects_parameter()` reports whether the action consumes a value token.
- `invoke()` runs a no-value action. On `action_with_param<T>`, it throws
  `std::runtime_error`.
- `invoke_with_parameter(param)` runs a value action. On
  `action_no_param`, the parameter is ignored.
- `get_trait_hints()` returns `{format_hint, purpose_hint}` for typed actions
  when the parser trait exposes those members.
- `clone()` returns a heap-allocated copy used by `argument`.

### `action_with_param<T>`

```cpp
template <typename T>
class action_with_param : public action_base {
public:
    using parameter_type = T;

    explicit action_with_param(std::function<void(T const&)> const& handler);
    void invoke(T const& arg) const;
    [[nodiscard]] bool expects_parameter() const override;
    void invoke() const override;
    void invoke_with_parameter(std::string const& param) const override;
    [[nodiscard]] std::pair<std::string, std::string> get_trait_hints() const override;
    [[nodiscard]] std::unique_ptr<action_base> clone() const override;
};
```

Adapts a `std::function<void(T const&)>` into a parser action. During
`invoke_with_parameter`, the raw string is converted with
`parser_trait<T>::parse(param)` and the parsed value is passed to the handler.
Parse failures are reported as `std::runtime_error` with trait-derived hints
when available.

### `action_no_param`

```cpp
class action_no_param : public action_base {
public:
    explicit action_no_param(std::function<void()> const& handler);

    void invoke() const override;
    [[nodiscard]] bool expects_parameter() const override;
    void invoke_with_parameter(std::string const& param) const override;
    [[nodiscard]] std::pair<std::string, std::string> get_trait_hints() const override;
    [[nodiscard]] std::unique_ptr<action_base> clone() const override;
};
```

Adapts a `std::function<void()>` into a flag-style action.
`expects_parameter()` returns `false`; `invoke_with_parameter(...)` ignores the
parameter and runs the no-value handler.

### `argument_parser::helpers`

```cpp
namespace argument_parser::helpers {
    template <typename T>
    action_with_param<T>
    make_action(std::function<void(T const&)> const& function);

    action_no_param
    make_action(std::function<void()> const& function);
}
```

Factory helpers for creating action objects used by `base_parser` and v2.

### `argument`

```cpp
class argument {
public:
    argument();

    template <typename ActionType>
    argument(int id, std::string name, ActionType const& action);

    argument(argument const& other);
    argument& operator=(argument const& other);
    argument(argument&& other) noexcept = default;
    argument& operator=(argument&& other) noexcept = default;

    [[nodiscard]] bool is_required() const;
    [[nodiscard]] std::string get_name() const;
    [[nodiscard]] bool is_invoked() const;
    [[nodiscard]] bool expects_parameter() const;
    [[nodiscard]] std::string get_help_text() const;
    [[nodiscard]] bool is_positional() const;
    [[nodiscard]] bool is_positional_accumulator() const;
    [[nodiscard]] std::optional<int> get_position_index() const;
};
```

`argument` is the public descriptor returned by parser lookup operations. It is
copyable; copies clone their stored action.

- `is_required()` reports whether parsing must see this argument.
- `get_name()` returns the registered display name. Named options are stored as
  `"<short>|<long>"`; positional arguments use their positional name.
- `is_invoked()` reports whether the action ran during `handle_arguments(...)`.
- `expects_parameter()` mirrors the stored action.
- `get_help_text()` returns the registration help text.
- `is_positional()` identifies positional arguments.
- `is_positional_accumulator()` is true for positional accumulators.
- `get_position_index()` returns the explicit requested position, or
  `std::nullopt`.

### `base_parser`

```cpp
class base_parser {
public:
    template <typename T>
    void add_argument(
        std::string const& short_arg,
        std::string const& long_arg,
        std::string const& help_text,
        action_with_param<T> const& action,
        bool required);

    template <typename T>
    void add_argument(
        std::string const& short_arg,
        std::string const& long_arg,
        std::string const& help_text,
        bool required);

    void add_argument(
        std::string const& short_arg,
        std::string const& long_arg,
        std::string const& help_text,
        action_no_param const& action,
        bool required);

    void add_argument(
        std::string const& short_arg,
        std::string const& long_arg,
        std::string const& help_text,
        bool required);

    template <typename T>
    void add_positional_argument(
        std::string const& name,
        std::string const& help_text,
        action_with_param<T> const& action,
        bool required,
        std::optional<int> position = std::nullopt);

    template <typename T>
    void add_positional_argument(
        std::string const& name,
        std::string const& help_text,
        bool required,
        std::optional<int> position = std::nullopt);

    template <typename T>
    void add_positional_accumulator(
        std::string const& name,
        std::string const& help_text,
        action_with_param<T> const& action,
        bool required,
        std::optional<int> position = std::nullopt);

    void on_complete(std::function<void(base_parser const&)> const& action);

    template <typename T>
    std::optional<T> get_optional(std::string const& arg) const;

    [[nodiscard]] std::string build_help_text(
        std::initializer_list<conventions::convention const* const> convention_types) const;

    argument& get_argument(conventions::parsed_argument const& arg);
    [[nodiscard]] std::optional<int> find_argument_id(std::string const& arg) const;
    void handle_arguments(
        std::initializer_list<conventions::convention const* const> convention_types);
    void display_help(
        std::initializer_list<conventions::convention const* const> convention_types) const;

protected:
    base_parser() = default;
};
```

`base_parser` owns registrations, parsed command-line tokens, stored values,
required-argument checks, and completion hooks. Its constructor is protected;
users normally instantiate a concrete parser from `<argparse>` or a platform
header. Platform parser constructors populate `program_name` and
`parsed_arguments` from the native process arguments.

`base_parser` is not thread-safe. `handle_arguments(...)` must be called on the
same thread that created the parser or it throws `std::runtime_error`.

#### Named Arguments

Use `add_argument(...)` for short and/or long options. The v1 API uses `"-"` as
the sentinel for an omitted short or long name.

```cpp
argument_parser::parser parser;

parser.add_argument<std::string>(
    "o",
    "output",
    "Output file.",
    argument_parser::helpers::make_action<std::string>(
        [](std::string const& value) {
            // use value
        }),
    true);

parser.add_argument(
    "v",
    "verbose",
    "Enable verbose output.",
    false);
```

Registration fails with `std::runtime_error("The key already exists!")` if the
short or long key is already registered. Because duplicate checks also see the
`"-"` sentinel, avoid registering multiple v1 arguments that pass `"-"` for the
same side unless you have verified that behavior.

The overloads behave as follows:

- `add_argument<T>(..., action_with_param<T> const& action, bool required)`
  parses a value and invokes the supplied typed action.
- `add_argument<T>(..., bool required)` parses a value and stores it internally
  for `get_optional<T>(...)`.
- `add_argument(..., action_no_param const& action, bool required)`
  invokes a no-value action.
- `add_argument(..., bool required)` stores `true` internally when the option is
  present.

#### Positional Arguments

```cpp
parser.add_positional_argument<std::string>(
    "input",
    "Input path.",
    true,
    0);

parser.add_positional_argument<int>(
    "count",
    "Number of iterations.",
    argument_parser::helpers::make_action<int>(
        [](int value) {
            // use value
        }),
    false);
```

Positional arguments are matched against tokens that do not match any supplied
convention. You can provide an explicit zero-based `position`, or omit it to use
the next available positional slot.

`add_positional_argument<T>(..., bool required, ...)` stores the parsed value
internally under the positional name. The action overload invokes the supplied
typed action.

`add_positional_accumulator<T>(...)` registers a typed positional action that can
consume repeated positional tokens at its slot. Only one positional accumulator
is allowed, and it must be the last positional argument. Duplicate names,
negative explicit positions, or invalid accumulator placement throw
`std::runtime_error`.

#### Parsing

```cpp
parser.handle_arguments({
    &argument_parser::conventions::gnu_argument_convention,
    &argument_parser::conventions::gnu_equal_argument_convention,
    &argument_parser::conventions::windows_argument_convention,
    &argument_parser::conventions::windows_equal_argument_convention,
});
```

`handle_arguments(...)` invokes matching actions, records stored values, checks
required arguments, then runs completion hooks. For each token, conventions are
tried in order. If no convention matches, the token is consumed as the next
positional value when one is available. `--` forces all following tokens to be
treated as positional values.

Unknown options, missing values, parse failures, unexpected positional values,
and action errors are reported with `std::runtime_error`. Missing required
arguments print diagnostics and help text, then call `std::exit(1)`.

If a registered argument named `h` or `help` is found, the parser invokes that
help argument and returns from argument invocation early. Required checks and
completion hooks still run after invocation returns.

#### Lookup And Help

```cpp
if (auto output = parser.get_optional<std::string>("output")) {
    // *output is the stored value
}

auto id = parser.find_argument_id("output");
std::string help = parser.build_help_text({
    &argument_parser::conventions::gnu_argument_convention,
});
parser.display_help({
    &argument_parser::conventions::gnu_argument_convention,
});
```

`get_optional<T>(arg)` returns a stored value for a registered short name, long
name, or positional name. It returns `std::nullopt` when no argument by that name
exists or when no value was stored. If a value exists but `T` does not match the
stored type, `std::any_cast<T>` throws `std::bad_any_cast`.

`find_argument_id(arg)` returns the internal integer id for a registered short
name, long name, or positional name. `get_argument(parsed_argument)` resolves a
parsed convention result to an `argument&` and throws when no match exists.

`build_help_text(...)` returns formatted usage text. `display_help(...)` writes
that text to `std::cout`.

#### Completion Hooks

```cpp
parser.on_complete([](argument_parser::base_parser const& state) {
    // inspect state.get_optional<T>(...)
});
```

`on_complete(...)` appends a callback that runs after successful argument
invocation and required-argument checks.

## v2 API: `parser_v2.hpp`

The v2 API is declared under `argument_parser::v2`. It provides a
map/initializer-list facade over the v1 `base_parser`, while still accepting the
same convention objects and action helpers.

Typical use is through the platform alias from `<argparse>`:

```cpp
#include <argparse>

using namespace argument_parser::v2::flags;

int main() {
    argument_parser::v2::parser parser;

    parser.add_argument<int>({
        {LongArgument, "count"},
        {HelpText, "Number of items to process."},
        {Required, true},
    });

    parser.handle_arguments({
        &argument_parser::conventions::gnu_argument_convention,
        &argument_parser::conventions::windows_argument_convention,
    });

    auto count = parser.get_optional<int>("count");
}
```

### `enum class add_argument_flags`

These keys describe an argument passed to `v2::base_parser::add_argument`.

| Flag | Value type | Meaning |
| --- | --- | --- |
| `ShortArgument` | `std::string` | Short option name, such as `"v"`. |
| `LongArgument` | `std::string` | Long option name, such as `"verbose"`. |
| `Positional` | `std::string` | Registers a positional argument. When present, the positional registration path is used. |
| `Position` | `int` | Optional zero-based position for a positional argument. |
| `HelpText` | `std::string` | Help text shown in generated help output. |
| `Action` | `action_with_param<T>` or `action_no_param` | Callback invoked when the argument is found. |
| `Required` | `bool` | Marks the argument required when `true`. |
| `Reference` | `T*` | Stores a parsed typed value into an external object. Cannot be combined with `Action`. |
| `Accumulate` | `bool` or `T*` | Repeated-value mode. For `std::vector<U>`, each parsed `U` is appended. For `int`, each occurrence increments the count. |

The `argument_parser::v2::flags` namespace exposes inline constants with the
same names for concise initializer-list calls.

```cpp
using namespace argument_parser::v2::flags;

parser.add_argument({
    {ShortArgument, "q"},
    {HelpText, "Quiet mode."},
});
```

### `argument_parser::v2::deducers`

```cpp
template <typename, typename = void>
struct has_value_type;

template <typename T>
struct is_vector;

template <typename T>
constexpr bool is_vector_v = is_vector<T>::test();
```

These public type traits are used by v2 accumulator logic. `has_value_type`
detects `T::value_type`. `is_vector<T>::test()` and `is_vector_v<T>` are true
only when `T` is exactly a `std::vector` specialization.

### `argument_parser::v2::base_parser`

`v2::base_parser` is the v2 facade. Concrete platform parsers such as
`argument_parser::v2::linux_parser`, `macos_parser`, `windows_parser`, and the
portable `<argparse>` alias `argument_parser::v2::parser` derive from it.

The class privately inherits from the v1 `argument_parser::base_parser` and
re-exposes selected operations.

```cpp
template <typename T>
using typed_flag_value =
    std::variant<std::string, action_with_param<T>, bool, int, T*>;

using non_typed_flag_value =
    std::variant<std::string, action_no_param, bool, int>;

template <typename T>
using typed_argument_pair =
    std::pair<add_argument_flags, typed_flag_value<T>>;

using non_typed_argument_pair =
    std::pair<add_argument_flags, non_typed_flag_value>;
```

Use typed aliases for arguments that parse a value of `T`, run an
`action_with_param<T>`, store into a `T`, or accumulate values. Use non-typed
aliases for boolean flags and no-parameter actions.

#### Typed `add_argument`

```cpp
template <typename T>
void add_argument(
    std::unordered_map<add_argument_flags, typed_flag_value<T>> const& argument_pairs);

template <typename T>
void add_argument(std::initializer_list<typed_argument_pair<T>> const& pairs);
```

Registers a typed option or positional argument. For non-positional options,
provide at least one of `ShortArgument` or `LongArgument`. If only one name is
supplied, the other is internally set to `"-"` and not registered.

Without `Action`, `Reference`, or `Accumulate`, the parsed `T` is stored
internally and can be read with `get_optional<T>()`. For positional arguments,
include `Positional`; `Position` can place the argument at a specific zero-based
slot.

```cpp
int threshold = 0;

parser.add_argument<int>({
    {LongArgument, "threshold"},
    {Reference, &threshold},
    {HelpText, "Store the parsed threshold."},
});
```

#### Non-Typed `add_argument`

```cpp
void add_argument(
    std::unordered_map<add_argument_flags, non_typed_flag_value> const& argument_pairs);

void add_argument(std::initializer_list<non_typed_argument_pair> const& pairs);
```

Registers a non-typed option or positional argument. For options, provide at
least one of `ShortArgument` or `LongArgument`. Without `Action`, the argument
behaves as a boolean flag: when present, it stores `true` for
`get_optional<bool>()`.

With `Action`, provide an `action_no_param`. Non-typed arguments cannot use
`Reference` or `Accumulate`. A non-typed positional argument is stored as
`std::string`.

```cpp
parser.add_argument({
    {ShortArgument, "v"},
    {LongArgument, "verbose"},
    {HelpText, "Enable verbose output."},
});
```

#### Other Public Operations

```cpp
argument_parser::base_parser& to_v1();

void handle_arguments(
    std::initializer_list<conventions::convention const* const> convention_types);

template <typename T>
std::optional<T> get_optional(std::string const& arg);

using argument_parser::base_parser::display_help;
using argument_parser::base_parser::on_complete;
```

`to_v1()` returns a mutable reference to the underlying v1 parser interface.
`handle_arguments(...)`, `get_optional<T>(...)`, `display_help(...)`, and
`on_complete(...)` follow the v1 behavior described above.

### Accumulation

`Accumulate` has special typed behavior:

- `T = std::vector<U>` registers an argument that accepts repeated `U` values.
- `T = int` registers a counter-style option; each occurrence increments the
  count.
- `Accumulate` set to `true` stores the accumulated result internally and makes
  it available through `get_optional<T>()`.
- `Accumulate` set to `T*`, or `Accumulate` combined with `Reference`, writes
  directly to an external object.

For vector accumulators, the public argument type is `std::vector<U>`, but each
individual command-line occurrence is parsed as `U`.

```cpp
parser.add_argument<std::vector<int>>({
    {LongArgument, "include-id"},
    {Accumulate, true},
    {HelpText, "May be repeated."},
});

auto ids = parser.get_optional<std::vector<int>>("include-id");
```

## Builder API: `argument_builder.hpp`

The public builder API lives under `argument_parser::builder`. It provides a
staged fluent interface for registering arguments with
`argument_parser::v2::base_parser`.

```cpp
#include <argument_builder.hpp>
// or
#include <argparse>

using argument_parser::builder::new_argument;
```

### Public Types

`non_type` is a marker type used before a value-producing mode has been
selected. Application code normally does not need to name it.

`builder_mask` is a public namespace containing the compile-time machinery used
by `argument<mask, store_type>`. It exposes `mask_type`, `value_mode`,
`extra_capability`, capability constants such as `short_argument`,
`long_argument`, `positional`, `help_text`, `required`, `reference`,
`accumulate`, `count`, `store`, and `flag`, plus constexpr helpers such as
`bit`, `has`, `remove`, `replace`, `is_buildable`, and
`is_build_and_gettable`. These are mainly useful for type-level tests or
advanced wrappers.

### `container<store_type>`

```cpp
template <typename store_type>
class container;
```

Returned by `build_and_get(parser)`. It is populated after
`parser.handle_arguments(...)` completes.

- `store_type get() const` returns the stored value. Call only after the
  container is populated.
- `store_type& operator*()` dereferences the stored value.
- `store_type* operator->()` accesses members of the stored value.
- `operator bool()` reports whether a value is available.

For builder-managed storage, `build_and_get()` registers an `on_complete`
callback that reads the parsed value by lookup key. For
`accumulate(existing_vector)`, the container references the provided vector
directly.

### `argument<mask, store_type>`

```cpp
template <
    argument_parser::builder::builder_mask::mask_type mask =
        argument_parser::builder::builder_mask::initial,
    typename store_type = argument_parser::builder::non_type>
class argument;
```

The staged fluent builder. `mask` enables only valid next methods at compile
time. `store_type` is the type produced by the selected value mode. Most users
should create builders with `new_argument()`.

### Entry Points

```cpp
static auto argument<>::start() -> argument<builder_mask::initial>;
static inline auto new_argument();
```

`new_argument()` is equivalent to `argument<>::start()`.

```cpp
new_argument()
    .long_argument("output")
    .store<std::string>()
    .required()
    .build(parser);
```

### Identifier Methods

At least one identifier method must be used before `build(parser)` or
`build_and_get(parser)`.

```cpp
auto short_argument(std::string short_name) const -> argument<..., store_type>;
auto long_argument(std::string long_name) const -> argument<..., store_type>;
auto positional(std::string positional_name) const -> argument<..., store_type>;
auto position(int index) const -> argument<..., store_type>;
```

`short_argument(...)` and `long_argument(...)` set named option identifiers and
can be combined with each other. `positional(...)` registers a positional
argument and is mutually exclusive with named identifiers. `position(...)` is
available only after `positional(...)` and only once. If no value mode is
selected for a positional argument, `build(parser)` registers a `std::string`
positional value.

### Metadata Methods

```cpp
auto help_text(std::string help) const -> argument<..., store_type>;
auto required(bool value = true) const -> argument<..., store_type>;
```

`help_text(...)` sets generated help text and is single-use. If omitted, v2
generates default text, often using parser trait hints for typed value modes.

`required(...)` marks the argument required when `value` is `true`. There is no
public `setRequired(...)` method in `argument_builder.hpp`; the public fluent API
is `required(...)`.

### Terminal Value Modes

Exactly one terminal value mode can be selected. Once selected, the other modes
are removed from the builder type.

```cpp
template <typename T = std::string>
auto store() const -> argument<..., T>;

template <typename T>
auto reference(T& value) const -> argument<..., T>;

auto flag() const -> argument<..., bool>;

template <typename T = std::string>
auto accumulate() const -> argument<..., std::vector<T>>;

template <typename T>
auto accumulate(T& value) const -> argument<..., T>;

auto count() const -> argument<..., int>;
auto count(int& value) const -> argument<..., int>;

template <typename Callable>
auto action(Callable&& handler) const -> argument<..., non_type>;

template <typename T = std::string, typename Callable>
auto action(Callable&& handler) const -> argument<..., T>;
```

`store<T>()` parses a value and stores it in the parser for later lookup.
`store<void>()` is rejected at compile time.

`reference(T&)` parses a value and assigns it directly to an object that must
outlive parsing.

`flag()` registers a boolean presence flag. It is not available for positional
arguments. Named arguments also default to flag behavior when no terminal mode
is selected.

`accumulate<T>()` accepts repeated values and stores them as `std::vector<T>`.
`accumulate(existing)` appends repeated values into an existing vector. The
existing object must be a `std::vector` and must outlive parsing.

`count()` counts how many times a named argument appears. `count(int&)` writes
the count into an existing integer. `count()` is not available for positional
arguments.

`action(handler)` registers a no-value action when `handler` is invocable as
`void()`. For positional arguments, the builder adapts it through a
`std::string` positional action and ignores the parsed positional text.

`action<T>(handler)` registers a typed action when `handler` is invocable as
`void(T const&)`. `action<void>(...)` is rejected at compile time.

### Build Methods

```cpp
auto build(argument_parser::v2::base_parser& parser) const -> void;

auto build_and_get(argument_parser::v2::base_parser& parser) const
    -> container<store_type>;
```

`build(parser)` registers the configured argument with the parser. It requires
an identifier, dispatches to the selected terminal mode, and defaults to a named
boolean flag or `std::string` positional value when no terminal mode was
selected. It may propagate validation errors from `v2::base_parser`.

`build_and_get(parser)` registers the argument and returns a
`container<store_type>` for storable modes such as `store<T>()`, `flag()`,
default named flags, `accumulate<T>()`, `accumulate(vector&)`, `count()`, and
`count(int&)`. It throws `std::logic_error` for unsupported terminal modes such
as actions. The returned container is normally empty until
`parser.handle_arguments(...)` completes.

### Compile-Time Builder Rules

Invalid fluent chains fail to compile rather than failing at runtime.

- `build(...)` requires `short_argument(...)`, `long_argument(...)`, or
  `positional(...)`.
- `short_argument(...)` and `long_argument(...)` can be combined.
- `positional(...)` is mutually exclusive with named identifiers.
- `position(...)` is available only after `positional(...)` and only once.
- `help_text(...)` and `required(...)` are single-use.
- Terminal value modes are mutually exclusive.
- `flag()` and `count()` are not available for positional arguments.
- `store<void>()` and `action<void>(...)` are rejected with `static_assert`.
- `accumulate(existing)` requires `existing` to be a `std::vector`.

The header also exposes `argument_parser::builder::assertions`, a namespace of
small compile-time detection helpers and `static_assert` checks used to verify
these staged-builder rules. They are not needed for normal parser
configuration.

### Complete Builder Example

```cpp
#include <argparse>
#include <iostream>
#include <string>
#include <vector>

using argument_parser::builder::new_argument;

int main() {
    argument_parser::v2::parser parser(false);

    int threshold = 0;
    std::vector<int> ids;

    new_argument()
        .long_argument("file")
        .store<std::string>()
        .required()
        .help_text("Input file.")
        .build(parser);

    new_argument()
        .long_argument("threshold")
        .reference(threshold)
        .build(parser);

    new_argument()
        .long_argument("id")
        .accumulate(ids)
        .help_text("Repeatable numeric id.")
        .build(parser);

    auto verbose = new_argument()
        .short_argument("v")
        .count()
        .build_and_get(parser);

    parser.handle_arguments({
        &argument_parser::conventions::gnu_argument_convention,
        &argument_parser::conventions::windows_argument_convention,
    });

    if (auto file = parser.get_optional<std::string>("file")) {
        std::cout << "file: " << *file << '\n';
    }

    if (verbose) {
        std::cout << "verbosity: " << *verbose << '\n';
    }

    std::cout << "threshold: " << threshold << '\n';
    std::cout << "ids: " << ids.size() << '\n';
}
```

## Caveats

- The v1 API is retained as public compatibility surface, but the newer v2 and
  builder APIs are generally more ergonomic.
- Parser registration and parsing are single-threaded; `handle_arguments(...)`
  enforces the parser creation thread.
- `handle_arguments(...)` resets the current convention list after it returns
  or throws.
- Namespaces named `argument_parser::internal::*` are implementation details
  despite being present in public headers.
