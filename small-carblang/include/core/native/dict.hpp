#define NATIVE_DICT_CONTAINS                                                     \
    case NativeMethod::DICT_CONTAINS:                                            \
    {                                                                            \
        if (args.size() != 1 || !value_is<std::string>(args[0]))                 \
            throw std::runtime_error("contains() requires one string argument"); \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver);         \
        push(dict->contains(value_get<std::string>(args[0])));                   \
        break;                                                                   \
    }

#define NATIVE_DICT_REMOVE                                                     \
    case NativeMethod::DICT_REMOVE:                                            \
    {                                                                          \
        if (args.size() != 1 || !value_is<std::string>(args[0]))               \
            throw std::runtime_error("remove() requires one string argument"); \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver);       \
        push(dict->remove(value_get<std::string>(args[0])));                   \
        break;                                                                 \
    }

#define NATIVE_DICT_LENGTH                                               \
    case NativeMethod::DICT_LENGTH:                                      \
    {                                                                    \
        if (!args.empty())                                               \
            throw std::runtime_error("length() takes no arguments");     \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver); \
        push((double)dict->length());                                    \
        break;                                                           \
    }

#define NATIVE_DICT_KEYS                                                 \
    case NativeMethod::DICT_KEYS:                                        \
    {                                                                    \
        if (!args.empty())                                               \
            throw std::runtime_error("keys() takes no arguments");       \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver); \
        auto result = std::make_shared<Array>();                         \
        for (const auto &[k, _] : dict->entries)                         \
            result->elements.push_back(k);                               \
        push(result);                                                    \
        break;                                                           \
    }

#define NATIVE_DICT_VALUES                                               \
    case NativeMethod::DICT_VALUES:                                      \
    {                                                                    \
        if (!args.empty())                                               \
            throw std::runtime_error("values() takes no arguments");     \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver); \
        auto result = std::make_shared<Array>();                         \
        for (const auto &[_, v] : dict->entries)                         \
            result->elements.push_back(v);                               \
        push(result);                                                    \
        break;                                                           \
    }

#define NATIVE_DICT_POP                                                     \
    case NativeMethod::DICT_POP:                                            \
    {                                                                       \
        if (args.size() != 1 || !value_is<std::string>(args[0]))            \
            throw std::runtime_error("pop() requires one string argument"); \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver);    \
        const std::string &key = value_get<std::string>(args[0]);           \
        auto it = dict->entries.find(key);                                  \
        if (it == dict->entries.end())                                      \
        {                                                                   \
            push(nullptr);                                                  \
        }                                                                   \
        else                                                                \
        {                                                                   \
            Value val = it->second;                                         \
            dict->entries.erase(it);                                        \
            push(val);                                                      \
        }                                                                   \
        break;                                                              \
    }

#define NATIVE_DICT_COPY                                                 \
    case NativeMethod::DICT_COPY:                                        \
    {                                                                    \
        if (!args.empty())                                               \
            throw std::runtime_error("copy() takes no arguments");       \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver); \
        push(dict->copy());                                              \
        break;                                                           \
    }

#define NATIVE_DICT_CLEAR                                                \
    case NativeMethod::DICT_CLEAR:                                       \
    {                                                                    \
        if (!args.empty())                                               \
            throw std::runtime_error("clear() takes no arguments");      \
        auto &dict = value_get<std::shared_ptr<Dict>>(method->receiver); \
        dict->clear();                                                   \
        push(true);                                                      \
        break;                                                           \
    }
