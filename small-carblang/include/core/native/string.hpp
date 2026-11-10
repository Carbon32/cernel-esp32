#define NATIVE_PRIMITIVE_UPPER                                      \
    case NativeMethod::UPPER:                                       \
    {                                                               \
        if (!args.empty())                                          \
            throw std::runtime_error("upper() takes no arguments"); \
                                                                    \
        auto s = value_get<std::string>(method->receiver);          \
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);   \
        push(s);                                                    \
        break;                                                      \
    }

#define NATIVE_PRIMITIVE_LOWER                                      \
    case NativeMethod::LOWER:                                       \
    {                                                               \
        if (!args.empty())                                          \
            throw std::runtime_error("lower() takes no arguments"); \
                                                                    \
        auto s = value_get<std::string>(method->receiver);          \
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);   \
        push(s);                                                    \
        break;                                                      \
    }

#define NATIVE_PRIMITIVE_CAPITALIZE                                      \
    case NativeMethod::CAPITALIZE:                                       \
    {                                                                    \
        if (!args.empty())                                               \
            throw std::runtime_error("capitalize() takes no arguments"); \
                                                                         \
        auto s = value_get<std::string>(method->receiver);               \
        if (!s.empty())                                                  \
            s[0] = std::toupper(s[0]);                                   \
        push(s);                                                         \
        break;                                                           \
    }

#define NATIVE_PRIMITIVE_SWAPCASE                                    \
    case NativeMethod::SWAPCASE:                                     \
    {                                                                \
        if (!args.empty())                                           \
            throw std::runtime_error("swap() takes no arguments");   \
                                                                     \
        auto s = value_get<std::string>(method->receiver);           \
        for (char &c : s)                                            \
            c = std::islower(c) ? std::toupper(c) : std::tolower(c); \
        push(s);                                                     \
        break;                                                       \
    }

#define NATIVE_PRIMITIVE_FIND                                      \
    case NativeMethod::FIND:                                       \
    {                                                              \
        if (args.size() != 1)                                      \
            throw std::runtime_error("find() expects 1 argument"); \
                                                                   \
        auto s = value_get<std::string>(method->receiver);         \
        auto sub = value_get<std::string>(args[0]);                \
        auto pos = s.find(sub);                                    \
        push(pos == std::string::npos ? -1.0 : (double)pos);       \
        break;                                                     \
    }

#define NATIVE_PRIMITIVE_FIND_LAST                                        \
    case NativeMethod::FIND_LAST:                                         \
    {                                                                     \
        if (args.size() != 1)                                             \
            throw std::runtime_error("find_last() expects 1 argument");   \
                                                                          \
        auto s = value_get<std::string>(method->receiver);                \
        auto sub = value_get<std::string>(args[0]);                       \
                                                                          \
        auto pos = s.rfind(sub);                                          \
                                                                          \
        push(pos == std::string::npos ? -1.0 : static_cast<double>(pos)); \
        break;                                                            \
    }

#define NATIVE_PRIMITIVE_STR_FIRST                                           \
    case NativeMethod::STR_FIRST:                                            \
    {                                                                        \
        if (!args.empty())                                                   \
            throw std::runtime_error("first() takes no arguments");          \
                                                                             \
        auto s = value_get<std::string>(method->receiver);                   \
        push(s.empty() ? Value(nullptr) : Value(std::string(1, s.front()))); \
        break;                                                               \
    }

#define NATIVE_PRIMITIVE_STR_LAST                                           \
    case NativeMethod::STR_LAST:                                            \
    {                                                                       \
        if (!args.empty())                                                  \
            throw std::runtime_error("last() takes no arguments");          \
                                                                            \
        auto s = value_get<std::string>(method->receiver);                  \
        push(s.empty() ? Value(nullptr) : Value(std::string(1, s.back()))); \
        break;                                                              \
    }

#define NATIVE_PRIMITIVE_STARTS_WITH                                      \
    case NativeMethod::STARTS_WITH:                                       \
    {                                                                     \
        if (args.size() != 1)                                             \
            throw std::runtime_error("starts_with() expects 1 argument"); \
                                                                          \
        auto s = value_get<std::string>(method->receiver);                \
        auto p = value_get<std::string>(args[0]);                         \
        push(s.rfind(p, 0) == 0);                                         \
        break;                                                            \
    }

#define NATIVE_PRIMITIVE_ENDS_WITH                                      \
    case NativeMethod::ENDS_WITH:                                       \
    {                                                                   \
        if (args.size() != 1)                                           \
            throw std::runtime_error("ends_with() expects 1 argument"); \
                                                                        \
        auto s = value_get<std::string>(method->receiver);              \
        auto p = value_get<std::string>(args[0]);                       \
        push(s.size() >= p.size() &&                                    \
             s.compare(s.size() - p.size(), p.size(), p) == 0);         \
        break;                                                          \
    }

#define NATIVE_PRIMITIVE_IS_NUMBER                                      \
    case NativeMethod::IS_NUMBER:                                       \
    {                                                                   \
        if (!args.empty())                                              \
            throw std::runtime_error("is_number() takes no arguments"); \
                                                                        \
        auto s = value_get<std::string>(method->receiver);              \
        char *end;                                                      \
        std::strtod(s.c_str(), &end);                                   \
        push(end != s.c_str() && *end == '\0');                         \
        break;                                                          \
    }

#define NATIVE_PRIMITIVE_STR_TRIM                                  \
    case NativeMethod::STR_TRIM:                                   \
    {                                                              \
        if (!args.empty())                                         \
            throw std::runtime_error("trim() takes no arguments"); \
                                                                   \
        auto s = value_get<std::string>(method->receiver);         \
        auto ws = " \t\n\r";                                       \
        s.erase(0, s.find_first_not_of(ws));                       \
        s.erase(s.find_last_not_of(ws) + 1);                       \
        push(s);                                                   \
        break;                                                     \
    }

#define NATIVE_PRIMITIVE_TO_ARRAY                                      \
    case NativeMethod::TO_ARRAY:                                       \
    {                                                                  \
        if (!args.empty())                                             \
            throw std::runtime_error("to_array() takes no arguments"); \
                                                                       \
        auto s = value_get<std::string>(method->receiver);             \
        auto arr = std::make_shared<Array>();                          \
        for (char c : s)                                               \
            arr->elements.push_back(std::string(1, c));                \
        push(arr);                                                     \
        break;                                                         \
    }

#define NATIVE_PRIMITIVE_REPLACE                                          \
    case NativeMethod::REPLACE:                                           \
    {                                                                     \
        if (args.size() != 2)                                             \
            throw std::runtime_error("replace() expects 2 arguments");    \
                                                                          \
        auto s = value_get<std::string>(method->receiver);                \
        auto target = value_get<std::string>(args[0]);                    \
        auto replacement = value_get<std::string>(args[1]);               \
                                                                          \
        if (target.empty())                                               \
            throw std::runtime_error("replace() target cannot be empty"); \
                                                                          \
        size_t pos = 0;                                                   \
        while ((pos = s.find(target, pos)) != std::string::npos)          \
        {                                                                 \
            s.replace(pos, target.length(), replacement);                 \
            pos += replacement.length();                                  \
        }                                                                 \
                                                                          \
        push(s);                                                          \
        break;                                                            \
    }

#define NATIVE_PRIMITIVE_IS_SPACE                                                     \
    case NativeMethod::IS_SPACE:                                                      \
    {                                                                                 \
        if (!args.empty())                                                            \
            throw std::runtime_error("is_space() takes no arguments");                \
                                                                                      \
        auto s = value_get<std::string>(method->receiver);                            \
        if (s.size() != 1)                                                            \
            throw std::runtime_error("is_space() expects a single character string"); \
                                                                                      \
        char c = s[0];                                                                \
        bool result = std::isspace(static_cast<unsigned char>(c));                    \
                                                                                      \
        push(result ? true : false);                                                  \
        break;                                                                        \
    }

#define NATIVE_PRIMITIVE_IS_ALL_SPACES                                      \
    case NativeMethod::IS_ALL_SPACES:                                       \
    {                                                                       \
        if (!args.empty())                                                  \
            throw std::runtime_error("is_all_spaces() takes no arguments"); \
                                                                            \
        auto s = value_get<std::string>(method->receiver);                  \
                                                                            \
        bool all_spaces = true;                                             \
        for (char c : s)                                                    \
        {                                                                   \
            if (!std::isspace(static_cast<unsigned char>(c)))               \
            {                                                               \
                all_spaces = false;                                         \
                break;                                                      \
            }                                                               \
        }                                                                   \
                                                                            \
        push(all_spaces ? true : false);                                    \
        break;                                                              \
    }

#define NATIVE_PRIMITIVE_TO_NUMBER                                      \
    case NativeMethod::TO_NUMBER:                                       \
    {                                                                   \
        if (!args.empty())                                              \
            throw std::runtime_error("to_number() takes no arguments"); \
                                                                        \
        auto s = value_get<std::string>(method->receiver);              \
                                                                        \
        try                                                             \
        {                                                               \
            size_t idx;                                                 \
            double val = std::stod(s, &idx);                            \
                                                                        \
            if (idx != s.size())                                        \
                throw std::runtime_error("Invalid numeric string");     \
                                                                        \
            push(val);                                                  \
            break;                                                      \
        }                                                               \
        catch (const std::invalid_argument &)                           \
        {                                                               \
            throw std::runtime_error("Not a valid number");             \
        }                                                               \
        catch (const std::out_of_range &)                               \
        {                                                               \
            throw std::runtime_error("Number out of range");            \
        }                                                               \
    }

#define NATIVE_PRIMITIVE_STR_IS_EMPTY                                  \
    case NativeMethod::STR_IS_EMPTY:                                   \
    {                                                                  \
        if (!args.empty())                                             \
            throw std::runtime_error("is_empty() takes no arguments"); \
                                                                       \
        auto s = value_get<std::string>(method->receiver);             \
        bool result = s.empty();                                       \
        push(result ? true : false);                                   \
        break;                                                         \
    }

#define NATIVE_PRIMITIVE_STR_LENGTH                                  \
    case NativeMethod::STR_LENGTH:                                   \
    {                                                                \
        if (!args.empty())                                           \
            throw std::runtime_error("length() takes no arguments"); \
                                                                     \
        auto s = value_get<std::string>(method->receiver);           \
        push(static_cast<double>(s.length()));                       \
        break;                                                       \
    }

#define NATIVE_PRIMITIVE_STR_SLICE                                           \
    case NativeMethod::STR_SLICE:                                            \
    {                                                                        \
        if (args.size() < 2 || args.size() > 2)                              \
            throw std::runtime_error("slice() expects 2 arguments");         \
                                                                             \
        auto s = value_get<std::string>(method->receiver);                   \
        const double start_value = value_get<double>(args[0]);               \
        const double len_value = value_get<double>(args[1]);                 \
        if (!std::isfinite(start_value) || !std::isfinite(len_value) ||      \
            std::trunc(start_value) < 0 || std::trunc(len_value) < 0 ||      \
            std::trunc(start_value) >= static_cast<double>(s.size()))        \
            throw std::runtime_error("slice() invalid range");               \
        const size_t start = static_cast<size_t>(std::trunc(start_value));   \
        const size_t remaining = s.size() - start;                           \
        const size_t len = len_value >= static_cast<double>(remaining)       \
                               ? remaining                                   \
                               : static_cast<size_t>(std::trunc(len_value)); \
        std::string result = s.substr(start, len);                           \
        push(result);                                                        \
        break;                                                               \
    }

#define NATIVE_PRIMITIVE_STR_COUNT                                                  \
    case NativeMethod::STR_COUNT:                                                   \
    {                                                                               \
        if (args.size() != 1)                                                       \
            throw std::runtime_error("count() expects 1 argument");                 \
                                                                                    \
        auto s = value_get<std::string>(method->receiver);                          \
        auto sub = value_get<std::string>(args[0]);                                 \
                                                                                    \
        if (sub.empty())                                                            \
            throw std::runtime_error("count() the first argument cannot be empty"); \
                                                                                    \
        int count = 0;                                                              \
        size_t pos = 0;                                                             \
        while ((pos = s.find(sub, pos)) != std::string::npos)                       \
        {                                                                           \
            ++count;                                                                \
            pos += sub.size();                                                      \
        }                                                                           \
                                                                                    \
        push(static_cast<double>(count));                                           \
        break;                                                                      \
    }
