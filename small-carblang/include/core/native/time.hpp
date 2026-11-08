inline std::tm get_local_time(std::time_t tt)
{
    std::tm local_tm{};
#ifdef _WIN32
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    return local_tm;
}

#define NATIVE_GLOBALS_DATE                                              \
    case NativeMethod::DATE:                                             \
    {                                                                    \
        if (!args.empty())                                               \
            throw std::runtime_error("date() takes no arguments");       \
                                                                         \
        auto now = std::chrono::system_clock::now();                     \
        std::time_t tt = std::chrono::system_clock::to_time_t(now);      \
        std::tm local_tm = get_local_time(tt);                           \
                                                                         \
        auto dict = std::make_shared<Dict>();                            \
        dict->set("year", static_cast<double>(local_tm.tm_year + 1900)); \
        dict->set("month", static_cast<double>(local_tm.tm_mon + 1));    \
        dict->set("day", static_cast<double>(local_tm.tm_mday));         \
        dict->set("hour", static_cast<double>(local_tm.tm_hour));        \
        dict->set("minute", static_cast<double>(local_tm.tm_min));       \
        dict->set("second", static_cast<double>(local_tm.tm_sec));       \
                                                                         \
        push(dict);                                                      \
        break;                                                           \
    }

#define NATIVE_GLOBALS_SECOND_TO_MINUTE                                               \
    case NativeMethod::SECOND_TO_MINUTE:                                              \
    {                                                                                 \
        if (args.size() != 1)                                                         \
            throw std::runtime_error("second_to_minute() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                               \
            throw std::runtime_error("second_to_minute() argument must be a number"); \
                                                                                      \
        push(value_get<double>(args[0]) / 60.0);                                      \
        break;                                                                        \
    }

#define NATIVE_GLOBALS_SECOND_TO_HOUR                                               \
    case NativeMethod::SECOND_TO_HOUR:                                              \
    {                                                                               \
        if (args.size() != 1)                                                       \
            throw std::runtime_error("second_to_hour() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                             \
            throw std::runtime_error("second_to_hour() argument must be a number"); \
                                                                                    \
        push(value_get<double>(args[0]) / 3600.0);                                  \
        break;                                                                      \
    }

#define NATIVE_GLOBALS_SECOND_TO_DAY                                               \
    case NativeMethod::SECOND_TO_DAY:                                              \
    {                                                                              \
        if (args.size() != 1)                                                      \
            throw std::runtime_error("second_to_day() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                            \
            throw std::runtime_error("second_to_day() argument must be a number"); \
                                                                                   \
        push(value_get<double>(args[0]) / 86400.0);                                \
        break;                                                                     \
    }

#define NATIVE_GLOBALS_MINUTE_TO_SECOND                                               \
    case NativeMethod::MINUTE_TO_SECOND:                                              \
    {                                                                                 \
        if (args.size() != 1)                                                         \
            throw std::runtime_error("minute_to_second() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                               \
            throw std::runtime_error("minute_to_second() argument must be a number"); \
                                                                                      \
        push(value_get<double>(args[0]) * 60.0);                                      \
        break;                                                                        \
    }

#define NATIVE_GLOBALS_MINUTE_TO_HOUR                                               \
    case NativeMethod::MINUTE_TO_HOUR:                                              \
    {                                                                               \
        if (args.size() != 1)                                                       \
            throw std::runtime_error("minute_to_hour() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                             \
            throw std::runtime_error("minute_to_hour() argument must be a number"); \
                                                                                    \
        push(value_get<double>(args[0]) / 60.0);                                    \
        break;                                                                      \
    }

#define NATIVE_GLOBALS_MINUTE_TO_DAY                                               \
    case NativeMethod::MINUTE_TO_DAY:                                              \
    {                                                                              \
        if (args.size() != 1)                                                      \
            throw std::runtime_error("minute_to_day() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                            \
            throw std::runtime_error("minute_to_day() argument must be a number"); \
                                                                                   \
        push(value_get<double>(args[0]) / 1440.0);                                 \
        break;                                                                     \
    }

#define NATIVE_GLOBALS_HOUR_TO_SECOND                                               \
    case NativeMethod::HOUR_TO_SECOND:                                              \
    {                                                                               \
        if (args.size() != 1)                                                       \
            throw std::runtime_error("hour_to_second() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                             \
            throw std::runtime_error("hour_to_second() argument must be a number"); \
                                                                                    \
        push(value_get<double>(args[0]) * 3600.0);                                  \
        break;                                                                      \
    }

#define NATIVE_GLOBALS_HOUR_TO_MINUTE                                               \
    case NativeMethod::HOUR_TO_MINUTE:                                              \
    {                                                                               \
        if (args.size() != 1)                                                       \
            throw std::runtime_error("hour_to_minute() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                             \
            throw std::runtime_error("hour_to_minute() argument must be a number"); \
                                                                                    \
        push(value_get<double>(args[0]) * 60.0);                                    \
        break;                                                                      \
    }

#define NATIVE_GLOBALS_HOUR_TO_DAY                                               \
    case NativeMethod::HOUR_TO_DAY:                                              \
    {                                                                            \
        if (args.size() != 1)                                                    \
            throw std::runtime_error("hour_to_day() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                          \
            throw std::runtime_error("hour_to_day() argument must be a number"); \
                                                                                 \
        push(value_get<double>(args[0]) / 24.0);                                 \
        break;                                                                   \
    }

#define NATIVE_GLOBALS_LEAP_YEAR                                               \
    case NativeMethod::LEAP_YEAR:                                              \
    {                                                                          \
        if (args.size() != 1)                                                  \
            throw std::runtime_error("leap_year() expects 1 argument");        \
                                                                               \
        if (!value_is<double>(args[0]))                                        \
            throw std::runtime_error("leap_year() argument must be a number"); \
                                                                               \
        double v = value_get<double>(args[0]);                                 \
        if (std::floor(v) != v)                                                \
            throw std::runtime_error("leap_year() requires whole number");     \
                                                                               \
        int year = static_cast<int>(v);                                        \
                                                                               \
        bool is_leap = (year % 4 == 0 && year % 100 != 0) ||                   \
                       (year % 400 == 0);                                      \
                                                                               \
        push(is_leap ? true : false);                                          \
        break;                                                                 \
    }

#define NATIVE_GLOBALS_TODAY                                         \
    case NativeMethod::TODAY:                                        \
    {                                                                \
        if (!args.empty())                                           \
            throw std::runtime_error("today() expects 0 arguments"); \
                                                                     \
        std::time_t t = std::time(nullptr);                          \
        std::tm *now = std::localtime(&t);                           \
                                                                     \
        char buffer[11];                                             \
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);      \
                                                                     \
        push(std::string(buffer));                                   \
        break;                                                       \
    }
