#define NATIVE_PRIMITIVE_TO_STRING                                      \
    case NativeMethod::TO_STRING:                                       \
    {                                                                   \
        if (!args.empty())                                              \
            throw std::runtime_error("to_string() takes no arguments"); \
                                                                        \
        push(stringify(method->receiver));                              \
        break;                                                          \
    }

#define NATIVE_PRIMITIVE_TO_INT                                      \
    case NativeMethod::TO_INT:                                       \
    {                                                                \
        if (!args.empty())                                           \
            throw std::runtime_error("to_int() takes no arguments"); \
                                                                     \
        double val = value_get<double>(method->receiver);            \
                                                                     \
        int n = static_cast<int>(std::round(val));                   \
                                                                     \
        push(static_cast<double>(n));                                \
        break;                                                       \
    }

#define NATIVE_PRIMITIVE_BOOL_TO_INT                                 \
    case NativeMethod::BOOL_TO_INT:                                  \
    {                                                                \
        if (!args.empty())                                           \
            throw std::runtime_error("to_int() takes no arguments"); \
                                                                     \
        bool value = value_get<bool>(method->receiver);              \
        push(static_cast<double>(value));                            \
        break;                                                       \
    }
