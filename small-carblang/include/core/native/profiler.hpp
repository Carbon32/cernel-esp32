#define NATIVE_GLOBALS_PROFILE_START                                            \
    case NativeMethod::PROFILE_START:                                           \
    {                                                                           \
        if (args.size() != 1 || !value_is<std::string>(args[0]))                \
            throw std::runtime_error("profile_start() requires a string name"); \
        profiler.start(value_get<std::string>(args[0]));                        \
        push(nullptr);                                                          \
        break;                                                                  \
    }

#define NATIVE_GLOBALS_PROFILE_END                                            \
    case NativeMethod::PROFILE_END:                                           \
    {                                                                         \
        if (args.size() != 1 || !value_is<std::string>(args[0]))              \
            throw std::runtime_error("profile_end() requires a string name"); \
        profiler.end(value_get<std::string>(args[0]));                        \
        push(nullptr);                                                        \
        break;                                                                \
    }

#define NATIVE_GLOBALS_PROFILE_REPORT                                        \
    case NativeMethod::PROFILE_REPORT:                                       \
    {                                                                        \
        if (!args.empty())                                                   \
            throw std::runtime_error("profile_report() takes no arguments"); \
        push(profiler.report());                                             \
        break;                                                               \
    }

#define NATIVE_GLOBALS_PROFILE_RESET                                        \
    case NativeMethod::PROFILE_RESET:                                       \
    {                                                                       \
        if (!args.empty())                                                  \
            throw std::runtime_error("profile_reset() takes no arguments"); \
        profiler.reset();                                                   \
        push(nullptr);                                                      \
        break;                                                              \
    }
