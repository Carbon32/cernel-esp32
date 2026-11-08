#define NATIVE_GLOBALS_FILL                     \
    case NativeMethod::FILL:                    \
    {                                           \
        auto array = std::make_shared<Array>(); \
                                                \
        array->elements.reserve(args.size());   \
                                                \
        for (const auto &value : args)          \
            array->elements.push_back(value);   \
                                                \
        push(array);                            \
        break;                                  \
    }

#define NATIVE_GLOBALS_INIT                                                                    \
    case NativeMethod::INIT:                                                                   \
    {                                                                                          \
        if (args.size() != 2)                                                                  \
            throw std::runtime_error("init() takes exactly 2 arguments");                      \
                                                                                               \
        if (!value_is<double>(args[0]))                                                        \
            throw std::runtime_error("init() size must be a number");                          \
                                                                                               \
        size_t size = checked_size_count(args[0], Array{}.elements.max_size(), "init() size"); \
                                                                                               \
        const auto &value = args[1];                                                           \
                                                                                               \
        auto array = std::make_shared<Array>();                                                \
        array->elements.reserve(size);                                                         \
        for (size_t i = 0; i < size; ++i)                                                      \
            array->elements.push_back(value);                                                  \
                                                                                               \
        push(array);                                                                           \
        break;                                                                                 \
    }

#define NATIVE_GLOBALS_ARRAY_INPUT                                                                      \
    case NativeMethod::ARRAY_INPUT:                                                                     \
    {                                                                                                   \
        if (args.size() != 2)                                                                           \
            throw std::runtime_error("array_input() takes exactly 2 arguments");                        \
                                                                                                        \
        if (!value_is<double>(args[0]))                                                                 \
            throw std::runtime_error("array_input() count must be a number");                           \
                                                                                                        \
        if (!value_is<std::string>(args[1]))                                                            \
            throw std::runtime_error("array_input() prompt must be a string");                          \
                                                                                                        \
        size_t count = checked_size_count(args[0], Array{}.elements.max_size(), "array_input() count"); \
                                                                                                        \
        const std::string &prompt = value_get<std::string>(args[1]);                                    \
                                                                                                        \
        auto array = std::make_shared<Array>();                                                         \
        array->elements.reserve(count);                                                                 \
                                                                                                        \
        for (size_t i = 0; i < count; ++i)                                                              \
        {                                                                                               \
            std::cout << prompt;                                                                        \
            std::string line;                                                                           \
            std::getline(std::cin, line);                                                               \
            array->elements.push_back(line);                                                            \
        }                                                                                               \
                                                                                                        \
        push(array);                                                                                    \
        break;                                                                                          \
    }

#define NATIVE_ARRAY_PUSH                                                        \
    case NativeMethod::PUSH:                                                     \
    {                                                                            \
        if (args.empty())                                                        \
            throw std::runtime_error("push() needs at least one argument");      \
                                                                                 \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);       \
        array->elements.insert(array->elements.end(), args.begin(), args.end()); \
        push(true);                                                              \
        break;                                                                   \
    }

#define NATIVE_ARRAY_POP                                                   \
    case NativeMethod::POP:                                                \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("pop() takes no arguments");          \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (array->elements.empty())                                       \
        {                                                                  \
            push(nullptr);                                                 \
        }                                                                  \
        else                                                               \
        {                                                                  \
            push(array->elements.back());                                  \
            array->elements.pop_back();                                    \
        }                                                                  \
        break;                                                             \
    }

#define NATIVE_ARRAY_LENGTH                                                \
    case NativeMethod::LENGTH:                                             \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("length() takes no arguments");       \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        push((double)array->elements.size());                              \
        break;                                                             \
    }

#define NATIVE_ARRAY_IS_EMPTY                                              \
    case NativeMethod::IS_EMPTY:                                           \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("is_empty() takes no arguments");     \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        push(array->elements.empty());                                     \
        break;                                                             \
    }

#define NATIVE_ARRAY_CLEAR                                                 \
    case NativeMethod::CLEAR:                                              \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("clear() takes no arguments");        \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        array->elements.clear();                                           \
        push(true);                                                        \
        break;                                                             \
    }

#define NATIVE_ARRAY_CONTAINS                                              \
    case NativeMethod::CONTAINS:                                           \
    {                                                                      \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (args.size() != 1)                                              \
            throw std::runtime_error("contains() expects 1 argument");     \
                                                                           \
        bool found = false;                                                \
        for (auto &v : array->elements)                                    \
            if (v == args[0])                                              \
            {                                                              \
                found = true;                                              \
                break;                                                     \
            }                                                              \
                                                                           \
        push(found);                                                       \
        break;                                                             \
    }

#define NATIVE_ARRAY_INDEX_OF                                              \
    case NativeMethod::INDEX_OF:                                           \
    {                                                                      \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (args.size() != 1)                                              \
            throw std::runtime_error("first_index() expects 1 argument");  \
                                                                           \
        int idx = -1;                                                      \
        for (size_t i = 0; i < array->elements.size(); ++i)                \
        {                                                                  \
            if (array->elements[i] == args[0])                             \
            {                                                              \
                idx = (int)i;                                              \
                break;                                                     \
            }                                                              \
        }                                                                  \
                                                                           \
        push((double)idx);                                                 \
        break;                                                             \
    }

#define NATIVE_ARRAY_JOIN                                                  \
    case NativeMethod::JOIN:                                               \
    {                                                                      \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        std::string sep = args.empty()                                     \
                              ? ","                                        \
                              : value_get<std::string>(args[0]);           \
                                                                           \
        if (args.size() != 1)                                              \
            throw std::runtime_error("join() expects 1 argument");         \
                                                                           \
        std::ostringstream out;                                            \
        const auto &elems = array->elements;                               \
                                                                           \
        for (size_t i = 0; i < elems.size(); ++i)                          \
        {                                                                  \
            out << stringify(elems[i]);                                    \
            if (i + 1 < elems.size())                                      \
                out << sep;                                                \
        }                                                                  \
                                                                           \
        push(out.str());                                                   \
        break;                                                             \
    }

#define NATIVE_ARRAY_INSERT                                                                \
    case NativeMethod::INSERT:                                                             \
    {                                                                                      \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);                 \
        if (args.size() != 2)                                                              \
            throw std::runtime_error("insert() expects 2 arguments");                      \
                                                                                           \
        size_t idx = checked_sequence_index(args[0], array->elements.size() + 1, "Array"); \
                                                                                           \
        array->elements.insert(                                                            \
            array->elements.begin() + idx,                                                 \
            args[1]);                                                                      \
                                                                                           \
        push(true);                                                                        \
        break;                                                                             \
    }

#define NATIVE_ARRAY_REMOVE_AT                                                         \
    case NativeMethod::REMOVE_AT:                                                      \
    {                                                                                  \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);             \
        if (args.size() != 1)                                                          \
            throw std::runtime_error("remove() expects 1 argument");                   \
                                                                                       \
        const auto *number = value_get_if<double>(&args[0]);                           \
        if (!number || !std::isfinite(*number))                                        \
            throw std::runtime_error("Array index must be a finite number");           \
        double truncated = std::trunc(*number);                                        \
        if (truncated < 0 || truncated >= static_cast<double>(array->elements.size())) \
        {                                                                              \
            push(nullptr);                                                             \
            break;                                                                     \
        }                                                                              \
        size_t idx = static_cast<size_t>(truncated);                                   \
                                                                                       \
        Value v = array->elements[idx];                                                \
        array->elements.erase(                                                         \
            array->elements.begin() + idx);                                            \
                                                                                       \
        push(v);                                                                       \
        break;                                                                         \
    }

#define NATIVE_ARRAY_REVERSE                                               \
    case NativeMethod::REVERSE:                                            \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("reverse() takes no arguments");      \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        std::reverse(array->elements.begin(), array->elements.end());      \
        push(true);                                                        \
        break;                                                             \
    }

#define NATIVE_ARRAY_FIRST                                                 \
    case NativeMethod::FIRST:                                              \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("first() takes no arguments");        \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (array->elements.empty())                                       \
            push(nullptr);                                                 \
        else                                                               \
            push(array->elements.front());                                 \
        break;                                                             \
    }

#define NATIVE_ARRAY_LAST                                                  \
    case NativeMethod::LAST:                                               \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("last() takes no arguments");         \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (array->elements.empty())                                       \
            push(nullptr);                                                 \
        else                                                               \
            push(array->elements.back());                                  \
        break;                                                             \
    }

#define NATIVE_ARRAY_SLICE                                                                                                             \
    case NativeMethod::SLICE:                                                                                                          \
    {                                                                                                                                  \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);                                                             \
        if (args.size() < 1 || args.size() > 2)                                                                                        \
            throw std::runtime_error("slice() expects 1 or 2 arguments");                                                              \
                                                                                                                                       \
        size_t start = clamped_sequence_index(args[0], array->elements.size(), "Slice start");                                         \
        size_t end = args.size() == 2 ? clamped_sequence_index(args[1], array->elements.size(), "Slice end") : array->elements.size(); \
        if (end < start)                                                                                                               \
            end = start;                                                                                                               \
                                                                                                                                       \
        auto new_array = std::make_shared<Array>();                                                                                    \
        new_array->elements.insert(                                                                                                    \
            new_array->elements.begin(),                                                                                               \
            array->elements.begin() + start,                                                                                           \
            array->elements.begin() + end);                                                                                            \
                                                                                                                                       \
        push(new_array);                                                                                                               \
        break;                                                                                                                         \
    }

#define NATIVE_ARRAY_COPY                                                  \
    case NativeMethod::COPY:                                               \
    {                                                                      \
        if (!args.empty())                                                 \
            throw std::runtime_error("copy() takes no arguments");         \
                                                                           \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        auto new_array = std::make_shared<Array>();                        \
        new_array->elements = array->elements;                             \
        push(new_array);                                                   \
        break;                                                             \
    }

#define NATIVE_ARRAY_CONCAT                                                                                          \
    case NativeMethod::CONCAT:                                                                                       \
    {                                                                                                                \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);                                           \
        if (args.size() != 1)                                                                                        \
            throw std::runtime_error("concat() expects 1 argument");                                                 \
                                                                                                                     \
        if (!value_is<std::shared_ptr<Array>>(args[0]))                                                              \
            throw std::runtime_error("concat() argument must be an array");                                          \
                                                                                                                     \
        auto other = value_get<std::shared_ptr<Array>>(args[0]);                                                     \
        auto new_array = std::make_shared<Array>();                                                                  \
        new_array->elements.reserve(checked_combined_size(                                                           \
            array->elements.size(), other->elements.size(), new_array->elements.max_size(), "Array concatenation")); \
        new_array->elements = array->elements;                                                                       \
        new_array->elements.insert(new_array->elements.end(), other->elements.begin(), other->elements.end());       \
                                                                                                                     \
        push(new_array);                                                                                             \
        break;                                                                                                       \
    }

#define NATIVE_ARRAY_SWAP                                                            \
    case NativeMethod::SWAP:                                                         \
    {                                                                                \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);           \
        if (args.size() != 2)                                                        \
            throw std::runtime_error("swap() expects 2 arguments");                  \
                                                                                     \
        size_t i = checked_sequence_index(args[0], array->elements.size(), "Array"); \
        size_t j = checked_sequence_index(args[1], array->elements.size(), "Array"); \
                                                                                     \
        std::swap(array->elements[i],                                                \
                  array->elements[j]);                                               \
                                                                                     \
        push(true);                                                                  \
        break;                                                                       \
    }

#define NATIVE_ARRAY_COUNT                                                 \
    case NativeMethod::COUNT:                                              \
    {                                                                      \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (args.size() != 1)                                              \
            throw std::runtime_error("count() expects 1 argument");        \
                                                                           \
        int c = 0;                                                         \
        for (auto &v : array->elements)                                    \
            if (v == args[0])                                              \
                c++;                                                       \
                                                                           \
        push((double)c);                                                   \
        break;                                                             \
    }

#define NATIVE_ARRAY_EQUALS                                                \
    case NativeMethod::EQUALS:                                             \
    {                                                                      \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (args.size() != 1 ||                                            \
            !value_is<std::shared_ptr<Array>>(args[0]))                    \
        {                                                                  \
            push(false);                                                   \
            break;                                                         \
        }                                                                  \
                                                                           \
        auto other = value_get<std::shared_ptr<Array>>(args[0]);           \
        auto &a = array->elements;                                         \
        auto &b = other->elements;                                         \
        bool res = true;                                                   \
                                                                           \
        if (a.size() != b.size())                                          \
        {                                                                  \
            res = false;                                                   \
            break;                                                         \
        }                                                                  \
                                                                           \
        for (size_t i = 0; i < a.size(); ++i)                              \
        {                                                                  \
            if (res != false)                                              \
            {                                                              \
                if (!value_shallow_equal(a[i], b[i]))                      \
                {                                                          \
                    res = false;                                           \
                    break;                                                 \
                }                                                          \
            }                                                              \
        }                                                                  \
                                                                           \
        push(res);                                                         \
        break;                                                             \
    }

#define NATIVE_ARRAY_LAST_INDEX_OF                                         \
    case NativeMethod::LAST_INDEX_OF:                                      \
    {                                                                      \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver); \
        if (args.size() != 1)                                              \
            throw std::runtime_error("last_index() expects 1 argument");   \
                                                                           \
        int idx = -1;                                                      \
        for (int i = (int)array->elements.size() - 1; i >= 0; --i)         \
        {                                                                  \
            if (array->elements[i] == args[0])                             \
            {                                                              \
                idx = (int)i;                                              \
                break;                                                     \
            }                                                              \
        }                                                                  \
                                                                           \
        push((double)idx);                                                 \
        break;                                                             \
    }

#define NATIVE_ARRAY_SUM                                                        \
    case NativeMethod::SUM:                                                     \
    {                                                                           \
        if (!args.empty())                                                      \
            throw std::runtime_error("sum() takes no arguments");               \
                                                                                \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);      \
        double total = 0;                                                       \
        for (auto &v : array->elements)                                         \
        {                                                                       \
            if (!value_is<double>(v))                                           \
                throw std::runtime_error("sum() only works on numeric arrays"); \
            total += value_get<double>(v);                                      \
        }                                                                       \
        push(total);                                                            \
        break;                                                                  \
    }

#define NATIVE_ARRAY_MIN                                                        \
    case NativeMethod::MIN:                                                     \
    {                                                                           \
        if (!args.empty())                                                      \
            throw std::runtime_error("min() takes no arguments");               \
                                                                                \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);      \
        if (array->elements.empty())                                            \
            throw std::runtime_error("min() of empty array");                   \
                                                                                \
        double min_val = std::numeric_limits<double>::max();                    \
        for (auto &v : array->elements)                                         \
        {                                                                       \
            if (!value_is<double>(v))                                           \
                throw std::runtime_error("min() only works on numeric arrays"); \
            min_val = std::min(min_val, value_get<double>(v));                  \
        }                                                                       \
        push(min_val);                                                          \
        break;                                                                  \
    }

#define NATIVE_ARRAY_MAX                                                        \
    case NativeMethod::MAX:                                                     \
    {                                                                           \
        if (!args.empty())                                                      \
            throw std::runtime_error("max() takes no arguments");               \
                                                                                \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);      \
        if (array->elements.empty())                                            \
            throw std::runtime_error("max() of empty array");                   \
                                                                                \
        double max_val = std::numeric_limits<double>::lowest();                 \
        for (auto &v : array->elements)                                         \
        {                                                                       \
            if (!value_is<double>(v))                                           \
                throw std::runtime_error("max() only works on numeric arrays"); \
            max_val = std::max(max_val, value_get<double>(v));                  \
        }                                                                       \
        push(max_val);                                                          \
        break;                                                                  \
    }

#define NATIVE_ARRAY_AVERAGE                                                        \
    case NativeMethod::AVERAGE:                                                     \
    {                                                                               \
        if (!args.empty())                                                          \
            throw std::runtime_error("average() takes no arguments");               \
                                                                                    \
        auto &array = value_get<std::shared_ptr<Array>>(method->receiver);          \
        if (array->elements.empty())                                                \
            throw std::runtime_error("average() of empty array");                   \
                                                                                    \
        double total = 0;                                                           \
        for (auto &v : array->elements)                                             \
        {                                                                           \
            if (!value_is<double>(v))                                               \
                throw std::runtime_error("average() only works on numeric arrays"); \
            total += value_get<double>(v);                                          \
        }                                                                           \
        push(total / array->elements.size());                                       \
        break;                                                                      \
    }

#define NATIVE_ARRAY_TRIM                                                                      \
    case NativeMethod::TRIM:                                                                   \
    {                                                                                          \
        auto &array = value_get<std::shared_ptr<Array>>(                                       \
            method->receiver);                                                                 \
        if (args.size() != 1)                                                                  \
            throw std::runtime_error("trim() expects 1 argument");                             \
                                                                                               \
        size_t n = clamped_sequence_index(args[0], array->elements.size(), "Array trim size"); \
        array->elements.resize(n);                                                             \
                                                                                               \
        push(true);                                                                            \
        break;                                                                                 \
    }
