#define NATIVE_GLOBALS_FORMAT                                                          \
    case NativeMethod::FORMAT:                                                         \
    {                                                                                  \
        if (args.empty())                                                              \
            throw std::runtime_error("format() requires at least a format string");    \
                                                                                       \
        if (!value_is<std::string>(args[0]))                                           \
            throw std::runtime_error("format() first argument must be a string");      \
                                                                                       \
        std::string fmt = value_get<std::string>(args[0]);                             \
        std::string result;                                                            \
        size_t arg_index = 1;                                                          \
                                                                                       \
        for (size_t i = 0; i < fmt.size(); ++i)                                        \
        {                                                                              \
            if (fmt[i] == '%')                                                         \
            {                                                                          \
                if (i + 1 < fmt.size() && fmt[i + 1] == '%')                           \
                {                                                                      \
                    result += '%';                                                     \
                    ++i;                                                               \
                }                                                                      \
                else                                                                   \
                {                                                                      \
                    if (arg_index >= args.size())                                      \
                        throw std::runtime_error("Not enough arguments for format()"); \
                                                                                       \
                    result += stringify(args[arg_index++]);                            \
                }                                                                      \
            }                                                                          \
            else                                                                       \
            {                                                                          \
                result += fmt[i];                                                      \
            }                                                                          \
        }                                                                              \
                                                                                       \
        if (arg_index != args.size())                                                  \
            throw std::runtime_error("Too many arguments for format()");               \
                                                                                       \
        push(result);                                                                  \
        break;                                                                         \
    }

#define NATIVE_GLOBALS_PRINT                     \
    case NativeMethod::PRINT:                    \
    {                                            \
        std::string output;                      \
        for (size_t i = 0; i < args.size(); ++i) \
        {                                        \
            output += stringify(args[i]);        \
            if (i + 1 < args.size())             \
                output += ' ';                   \
        }                                        \
        write_output(output);                    \
        push(nullptr);                           \
        break;                                   \
    }

#define NATIVE_GLOBALS_PRINTLN                   \
    case NativeMethod::PRINTLN:                  \
    {                                            \
        std::string output;                      \
        for (size_t i = 0; i < args.size(); ++i) \
        {                                        \
            output += stringify(args[i]);        \
            if (i + 1 < args.size())             \
                output += ' ';                   \
        }                                        \
        output += "\r\n";                        \
        write_output(output);                    \
        push(nullptr);                           \
        break;                                   \
    }

#define NATIVE_GLOBALS_PRINTF                                                          \
    case NativeMethod::PRINTF:                                                         \
    {                                                                                  \
        if (args.empty())                                                              \
            throw std::runtime_error("printf() requires at least a format string");    \
                                                                                       \
        if (!value_is<std::string>(args[0]))                                           \
            throw std::runtime_error("printf() first argument must be a string");      \
                                                                                       \
        std::string fmt = value_get<std::string>(args[0]);                             \
        std::string output;                                                            \
        size_t arg_index = 1;                                                          \
                                                                                       \
        for (size_t i = 0; i < fmt.size(); ++i)                                        \
        {                                                                              \
            if (fmt[i] == '%')                                                         \
            {                                                                          \
                if (i + 1 < fmt.size() && fmt[i + 1] == '%')                           \
                {                                                                      \
                    output += '%';                                                     \
                    ++i;                                                               \
                }                                                                      \
                else                                                                   \
                {                                                                      \
                    if (arg_index >= args.size())                                      \
                        throw std::runtime_error("Not enough arguments for printf()"); \
                                                                                       \
                    output += stringify(args[arg_index++]);                            \
                }                                                                      \
            }                                                                          \
            else                                                                       \
            {                                                                          \
                output += fmt[i];                                                      \
            }                                                                          \
        }                                                                              \
                                                                                       \
        if (arg_index != args.size())                                                  \
            throw std::runtime_error("Too many arguments for printf()");               \
                                                                                       \
        write_output(output);                                                          \
        push(nullptr);                                                                 \
        break;                                                                         \
    }

#define NATIVE_GLOBALS_INPUT                                                \
    case NativeMethod::INPUT:                                               \
    {                                                                       \
        std::string prompt;                                                 \
                                                                            \
        if (!args.empty())                                                  \
        {                                                                   \
            if (!value_is<std::string>(args[0]))                            \
                throw std::runtime_error("scan() prompt must be a string"); \
            prompt = value_get<std::string>(args[0]);                       \
        }                                                                   \
                                                                            \
        std::cout << prompt;                                                \
        std::string line;                                                   \
        std::getline(std::cin, line);                                       \
                                                                            \
        push(line);                                                         \
        break;                                                              \
    }

#define NATIVE_GLOBALS_PARSE_JSON                                  \
    case NativeMethod::PARSE_JSON:                                 \
    {                                                              \
        if (args.size() != 1 || !value_is<std::string>(args[0]))   \
            throw std::runtime_error("parse() requires a string"); \
                                                                   \
        JsonParser parser(value_get<std::string>(args[0]));        \
        push(parser.parse());                                      \
        break;                                                     \
    }

#define NATIVE_GLOBALS_STRINGIFY                                              \
    case NativeMethod::STRINGIFY:                                             \
    {                                                                         \
        if (args.size() != 1)                                                 \
            throw std::runtime_error("stringify() takes exactly 1 argument"); \
                                                                              \
        push(stringify(args[0]));                                             \
        break;                                                                \
    }

#define NATIVE_GLOBALS_TO_JSON                                            \
    case NativeMethod::TO_JSON:                                           \
    {                                                                     \
        if (args.size() != 1)                                             \
            throw std::runtime_error("json() requires 1 argument");       \
        if (!value_is<std::shared_ptr<Dict>>(args[0]))                    \
            throw std::runtime_error("json() only accepts dictionaries"); \
                                                                          \
        push(json_stringify(args[0]));                                    \
        break;                                                            \
    }

#define NATIVE_GLOBALS_PARSE_CSV                                       \
    case NativeMethod::PARSE_CSV:                                      \
    {                                                                  \
        if (args.size() != 1 || !value_is<std::string>(args[0]))       \
            throw std::runtime_error("parse_csv() requires a string"); \
                                                                       \
        CsvParser parser(value_get<std::string>(args[0]));             \
        auto raw = parser.parse();                                     \
        auto outer = std::make_shared<Array>();                        \
        for (const auto &row : raw)                                    \
        {                                                              \
            auto inner = std::make_shared<Array>();                    \
            for (const auto &cell : row)                               \
            {                                                          \
                inner->elements.push_back(cell);                       \
            }                                                          \
            outer->elements.push_back(inner);                          \
        }                                                              \
        push(outer);                                                   \
        break;                                                         \
    }

#define NATIVE_GLOBALS_TO_CSV                                           \
    case NativeMethod::TO_CSV:                                          \
    {                                                                   \
        if (args.size() != 1)                                           \
            throw std::runtime_error("csv() requires 1 argument");      \
                                                                        \
        if (!value_is<std::shared_ptr<Array>>(args[0]))                 \
            throw std::runtime_error("csv() expects an array of rows"); \
                                                                        \
        push(csv_stringify(args[0]));                                   \
        break;                                                          \
    }
