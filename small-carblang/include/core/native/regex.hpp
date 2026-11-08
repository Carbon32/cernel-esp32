#define NATIVE_GLOBALS_MATCH                                                           \
    case NativeMethod::MATCH:                                                          \
    {                                                                                  \
        if (args.size() != 2)                                                          \
            throw std::runtime_error("match() expects 2 arguments: pattern and text"); \
                                                                                       \
        if (!value_is<std::string>(args[0]) ||                                         \
            !value_is<std::string>(args[1]))                                           \
            throw std::runtime_error("match() arguments must be strings");             \
                                                                                       \
        const std::string &pattern = value_get<std::string>(args[0]);                  \
        const std::string &text = value_get<std::string>(args[1]);                     \
                                                                                       \
        std::regex re;                                                                 \
        try                                                                            \
        {                                                                              \
            re = std::regex(pattern);                                                  \
        }                                                                              \
        catch (const std::regex_error &)                                               \
        {                                                                              \
            throw std::runtime_error("match() invalid regex pattern");                 \
        }                                                                              \
                                                                                       \
        auto array = std::make_shared<Array>();                                        \
                                                                                       \
        std::sregex_iterator begin(text.begin(), text.end(), re);                      \
        std::sregex_iterator end;                                                      \
                                                                                       \
        for (auto it = begin; it != end; ++it)                                         \
        {                                                                              \
            array->elements.push_back(it->str());                                      \
        }                                                                              \
                                                                                       \
        push(array);                                                                   \
        break;                                                                         \
    }
