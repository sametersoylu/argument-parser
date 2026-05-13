# TODO 1: After Parse callback [COMPLETED]
Currently interdependent arguments are a problem. In the current version we are handling them as closures. 
We create two stateful actions who are taking the state from the close over of the lambda. This is a little hacky/ugly. 
To fix this we could have a callback/on_parse_done event. An example usage could be; 
```cpp
parser.after_parse([&parser]() {
    auto grep_val = parser.get_optional<std::string>("grep");
    auto file_val = parser.get_optional<std::string>("file");
    
    if (grep_val && file_val) {
        grep(*grep_val, *file_val); 
    } else if (grep_val) {
        std::cerr << "grep requires file\n";
    }
});
```

After handle_parse(...) ends, we can call the on_parse_done callbacks. But this requires two things; 
1. Storable arguments. Those would be not actions, but stored values. So you could get them later on in the callback or somewhere else.
2. The on_parse_done callbacks should be called after all actions are invoked. But we should make this a toggle, so the callback only gets executed if the user explicitly wants it to be called. By default we would enable it. 

### Update: 29 March. 2026
Help text is clear, supports multi convention print, also provides hints for types that provides the hints in the traits.
Positional argument support is here. Also we now have some tests to ensure the library works as intended. 
Also introduce package cmake config to enable other developers to install the library and link against their executables. 

# TODO 2: Positional arguments | DONE 
Positional arguments are arguments that are not prefixed with a dash. They are usually the arguments that are passed to the program. 
For example, in the command `grep "pattern" file.txt`, "pattern" and "file.txt" are positional arguments.

# TODO 3: Separate headers from implementations | DONE 
Right now whole project is header only. Header only so far is fine but to avoid issues later on, it's better if we separate them. 

# TODO 4: Configure CMAKE | DONE 
Configure CMAKE to ensure the library can be built as a library and can be installed via CMAKE. This should make adoptation of this to existing projects easier with less headache. 

# TODO 5: Display help | DONE  
Display help doesn't reflect the conventions right now. Also it should come automatically, and should be allowed to overriden by user.

# TODO 6: Accumulate repeated calls | DONE
Add support to let users accumulate repeated calls to a flag. If the flag is called x times, the result should be x items stored in a vector, instead of an action doing it. 

# TODO 7: Defaults/Implicits
If given, an arguments default store value could be changed. If nothing was given use that value instead.

# TODO 8: Validators | DONE
If given, validate the argument before passing to the storage or action. If fail, let user decide fail loud or fail skip. 

# TODO 9: Subcommand/Subactions
Implement subcommand support. Users should be able to define subactions to the higher level action. For example, 
```cpp
parser.add_argument(
    {{ShortArgument, "g"}, {LongArgument, "get"}, {Action, get_}, {HelpText, "Gets <files, system_info, status>"}}
);
parser.add_argument(
    {{BaseArgument, "g"}, {ShortArgument, "f"}, {LongArgument, "files"}, {Action, get_files}, {HelpText, "Gets files"}}
);
...
```

# TODO 10: Reference capture | DONE
Reference capturing. 
```cpp
parser.add_argument<int>({
    {ShortArgument, "c"},
    {HelpText, "capture value"},
    {Reference, &captured_value},
});
```

# TODO 11: Builder | DONE
Implement type safe logic enforcing argument builder; 
```cpp
argument::start()
    .short_argument("e")
    .long_argument("echo")
    .help_text("Echo the parsed value.")
    .action(echo)
    .build(parser);
```
