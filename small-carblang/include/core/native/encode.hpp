#define NATIVE_GLOBALS_BASE                                                                            \
    case NativeMethod::BASE:                                                                           \
    {                                                                                                  \
        if (args.size() != 2)                                                                          \
            throw std::runtime_error("base() expects 2 arguments, number and base");                   \
                                                                                                       \
        if (!value_is<double>(args[0]) || !value_is<double>(args[1]))                                  \
            throw std::runtime_error("base() arguments must be numbers");                              \
                                                                                                       \
        double num = value_get<double>(args[0]);                                                       \
        double base_value = value_get<double>(args[1]);                                                \
        if (!std::isfinite(base_value) || std::floor(base_value) != base_value ||                      \
            base_value < 2 || base_value > 16)                                                         \
            throw std::runtime_error("base() only supports integer bases from 2 to 16");               \
        if (!std::isfinite(num) || num < static_cast<double>(std::numeric_limits<long long>::min()) || \
            num >= -static_cast<double>(std::numeric_limits<long long>::min()))                        \
            throw std::runtime_error("base() number is out of range");                                 \
        int base = static_cast<int>(base_value);                                                       \
                                                                                                       \
        if (base < 2 || base > 16)                                                                     \
            throw std::runtime_error("base() only supports bases from 2 to 16");                       \
                                                                                                       \
        long long n = static_cast<long long>(num);                                                     \
        bool negative = n < 0;                                                                         \
        if (negative)                                                                                  \
        {                                                                                              \
            if (n == std::numeric_limits<long long>::min())                                            \
                throw std::runtime_error("base() cannot represent the absolute value of LLONG_MIN");   \
            n = -n;                                                                                    \
        }                                                                                              \
                                                                                                       \
        std::string digits = "0123456789ABCDEF";                                                       \
        std::string result;                                                                            \
                                                                                                       \
        do                                                                                             \
        {                                                                                              \
            result = digits[n % base] + result;                                                        \
            n /= base;                                                                                 \
        } while (n > 0);                                                                               \
                                                                                                       \
        if (negative)                                                                                  \
            result = "-" + result;                                                                     \
        push(result);                                                                                  \
        break;                                                                                         \
    }
