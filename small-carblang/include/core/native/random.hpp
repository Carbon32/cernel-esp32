#define NATIVE_GLOBALS_RAND                                           \
    case NativeMethod::RAND:                                          \
    {                                                                 \
        if (!args.empty())                                            \
            throw std::runtime_error("random() takes no arguments");  \
                                                                      \
        static std::mt19937 rng{std::random_device{}()};              \
        static std::uniform_real_distribution<double> dist(0.0, 1.0); \
                                                                      \
        push(dist(rng));                                              \
        break;                                                        \
    }

#define NATIVE_GLOBALS_RANDINT                                                             \
    case NativeMethod::RANDINT:                                                            \
    {                                                                                      \
        if (args.size() != 2)                                                              \
            throw std::runtime_error("random_integer() takes exactly 2 arguments");        \
                                                                                           \
        if (!value_is<double>(args[0]) ||                                                  \
            !value_is<double>(args[1]))                                                    \
            throw std::runtime_error("random_integer() arguments must be numbers");        \
                                                                                           \
        const double a_value = value_get<double>(args[0]);                                 \
        const double b_value = value_get<double>(args[1]);                                 \
        if (!std::isfinite(a_value) || !std::isfinite(b_value) ||                          \
            a_value < static_cast<double>(std::numeric_limits<int>::min()) ||              \
            b_value < static_cast<double>(std::numeric_limits<int>::min()) ||              \
            a_value >= -static_cast<double>(std::numeric_limits<int>::min()) ||            \
            b_value >= -static_cast<double>(std::numeric_limits<int>::min()))              \
            throw std::runtime_error("random_integer() arguments are out of range");       \
        int a = static_cast<int>(a_value);                                                 \
        int b = static_cast<int>(b_value);                                                 \
                                                                                           \
        if (a > b)                                                                         \
            throw std::runtime_error("random_integer() first argument must be <= second"); \
                                                                                           \
        static std::mt19937 rng{std::random_device{}()};                                   \
        std::uniform_int_distribution<int> dist(a, b);                                     \
                                                                                           \
        push(static_cast<double>(dist(rng)));                                              \
        break;                                                                             \
    }

#define NATIVE_GLOBALS_RANDOM_COLOR_RGB                                         \
    case NativeMethod::RANDOM_COLOR_RGB:                                        \
    {                                                                           \
        if (!args.empty())                                                      \
            throw std::runtime_error("random_color_rgb() expects 0 arguments"); \
                                                                                \
        static std::mt19937 rng(std::random_device{}());                        \
        static std::uniform_int_distribution<int> dist(0, 255);                 \
                                                                                \
        int r = dist(rng);                                                      \
        int g = dist(rng);                                                      \
        int b = dist(rng);                                                      \
                                                                                \
        auto array = std::make_shared<Array>();                                 \
        array->elements.push_back(static_cast<double>(r));                      \
        array->elements.push_back(static_cast<double>(g));                      \
        array->elements.push_back(static_cast<double>(b));                      \
                                                                                \
        push(array);                                                            \
        break;                                                                  \
    }

#define NATIVE_GLOBALS_RANDOM_COLOR_HEX                                         \
    case NativeMethod::RANDOM_COLOR_HEX:                                        \
    {                                                                           \
        if (!args.empty())                                                      \
            throw std::runtime_error("random_color_hex() expects 0 arguments"); \
                                                                                \
        static std::mt19937 rng(std::random_device{}());                        \
        static std::uniform_int_distribution<int> dist(0, 255);                 \
                                                                                \
        int r = dist(rng);                                                      \
        int g = dist(rng);                                                      \
        int b = dist(rng);                                                      \
                                                                                \
        std::stringstream ss;                                                   \
        ss << "#"                                                               \
           << std::uppercase                                                    \
           << std::hex                                                          \
           << std::setw(2) << std::setfill('0') << r                            \
           << std::setw(2) << std::setfill('0') << g                            \
           << std::setw(2) << std::setfill('0') << b;                           \
                                                                                \
        push(ss.str());                                                         \
        break;                                                                  \
    }