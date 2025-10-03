# TODO 1: After Parse callback
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

## Self reminder, implement this feature. It'd be helpful. 
