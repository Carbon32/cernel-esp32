#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NATIVE_GLOBALS_POW                                                            \
    case NativeMethod::POW:                                                           \
    {                                                                                 \
        if (args.size() != 2)                                                         \
            throw std::runtime_error("pow() expects 2 arguments: base and exponent"); \
                                                                                      \
        if (!value_is<double>(args[0]) ||                                             \
            !value_is<double>(args[1]))                                               \
            throw std::runtime_error("pow() arguments must be numbers");              \
                                                                                      \
        double base = value_get<double>(args[0]);                                     \
        double exp = value_get<double>(args[1]);                                      \
        push(std::pow(base, exp));                                                    \
        break;                                                                        \
    }

#define NATIVE_GLOBALS_SQRT                                               \
    case NativeMethod::SQRT:                                              \
    {                                                                     \
        if (args.size() != 1)                                             \
            throw std::runtime_error("sqrt() expects 1 argument");        \
                                                                          \
        if (!value_is<double>(args[0]))                                   \
            throw std::runtime_error("sqrt() argument must be a number"); \
                                                                          \
        double x = value_get<double>(args[0]);                            \
        if (x < 0)                                                        \
            throw std::runtime_error("sqrt() of negative number");        \
        push(std::sqrt(x));                                               \
        break;                                                            \
    }

#define NATIVE_GLOBALS_FACT                                                   \
    case NativeMethod::FACT:                                                  \
    {                                                                         \
        if (args.size() != 1)                                                 \
            throw std::runtime_error("fact() expects 1 argument");            \
                                                                              \
        if (!value_is<double>(args[0]))                                       \
            throw std::runtime_error("fact() argument must be a number");     \
                                                                              \
        double val = value_get<double>(args[0]);                              \
        if (std::floor(val) != val)                                           \
            throw std::runtime_error("fact() only works with whole numbers"); \
                                                                              \
        int n = static_cast<int>(val);                                        \
        if (n < 0)                                                            \
            throw std::runtime_error("fact() of negative number");            \
                                                                              \
        double r = 1;                                                         \
        for (int i = 2; i <= n; ++i)                                          \
            r *= i;                                                           \
                                                                              \
        push(r);                                                              \
        break;                                                                \
    }

#define NATIVE_GLOBALS_FLOOR                                               \
    case NativeMethod::FLOOR:                                              \
    {                                                                      \
        if (args.size() != 1)                                              \
            throw std::runtime_error("floor() expects 1 argument");        \
                                                                           \
        if (!value_is<double>(args[0]))                                    \
            throw std::runtime_error("floor() argument must be a number"); \
                                                                           \
        push(std::floor(value_get<double>(args[0])));                      \
        break;                                                             \
    }

#define NATIVE_GLOBALS_CEIL                                               \
    case NativeMethod::CEIL:                                              \
    {                                                                     \
        if (args.size() != 1)                                             \
            throw std::runtime_error("ceil() expects 1 argument");        \
                                                                          \
        if (!value_is<double>(args[0]))                                   \
            throw std::runtime_error("ceil() argument must be a number"); \
                                                                          \
        push(std::ceil(value_get<double>(args[0])));                      \
        break;                                                            \
    }

#define NATIVE_GLOBALS_ABS                                               \
    case NativeMethod::ABS:                                              \
    {                                                                    \
        if (args.size() != 1)                                            \
            throw std::runtime_error("abs() expects 1 argument");        \
                                                                         \
        if (!value_is<double>(args[0]))                                  \
            throw std::runtime_error("abs() argument must be a number"); \
                                                                         \
        push(std::fabs(value_get<double>(args[0])));                     \
        break;                                                           \
    }

#define NATIVE_GLOBALS_LOG                                                 \
    case NativeMethod::LOG:                                                \
    {                                                                      \
        if (args.size() != 1)                                              \
            throw std::runtime_error("log() expects 1 argument");          \
                                                                           \
        if (!value_is<double>(args[0]))                                    \
            throw std::runtime_error("log() argument must be a number");   \
                                                                           \
        double x = value_get<double>(args[0]);                             \
        if (x <= 0)                                                        \
            throw std::runtime_error("log() domain error: x must be > 0"); \
                                                                           \
        push(std::log(x));                                                 \
        break;                                                             \
    }

#define NATIVE_GLOBALS_LOG10                                                 \
    case NativeMethod::LOG10:                                                \
    {                                                                        \
        if (args.size() != 1)                                                \
            throw std::runtime_error("log10() expects 1 argument");          \
                                                                             \
        if (!value_is<double>(args[0]))                                      \
            throw std::runtime_error("log10() argument must be a number");   \
                                                                             \
        double x = value_get<double>(args[0]);                               \
        if (x <= 0)                                                          \
            throw std::runtime_error("log10() domain error: x must be > 0"); \
                                                                             \
        push(std::log10(x));                                                 \
        break;                                                               \
    }

#define NATIVE_GLOBALS_LOG2                                                 \
    case NativeMethod::LOG2:                                                \
    {                                                                       \
        if (args.size() != 1)                                               \
            throw std::runtime_error("log2() expects 1 argument");          \
                                                                            \
        if (!value_is<double>(args[0]))                                     \
            throw std::runtime_error("log2() argument must be a number");   \
                                                                            \
        double x = value_get<double>(args[0]);                              \
        if (x <= 0)                                                         \
            throw std::runtime_error("log2() domain error: x must be > 0"); \
                                                                            \
        push(std::log2(x));                                                 \
        break;                                                              \
    }

#define NATIVE_GLOBALS_MATH_MAX                                              \
    case NativeMethod::MATH_MAX:                                             \
    {                                                                        \
        if (args.size() < 1)                                                 \
            throw std::runtime_error("max() expects at least 1 argument");   \
                                                                             \
        double m;                                                            \
        if (!value_is<double>(args[0]))                                      \
            throw std::runtime_error("max() arguments must be numbers");     \
        m = value_get<double>(args[0]);                                      \
                                                                             \
        for (size_t i = 1; i < args.size(); ++i)                             \
        {                                                                    \
            if (!value_is<double>(args[i]))                                  \
                throw std::runtime_error("max() arguments must be numbers"); \
            m = std::max(m, value_get<double>(args[i]));                     \
        }                                                                    \
                                                                             \
        push(m);                                                             \
        break;                                                               \
    }

#define NATIVE_GLOBALS_MATH_MIN                                              \
    case NativeMethod::MATH_MIN:                                             \
    {                                                                        \
        if (args.size() < 1)                                                 \
            throw std::runtime_error("min() expects at least 1 argument");   \
                                                                             \
        double m;                                                            \
        if (!value_is<double>(args[0]))                                      \
            throw std::runtime_error("min() arguments must be numbers");     \
        m = value_get<double>(args[0]);                                      \
                                                                             \
        for (size_t i = 1; i < args.size(); ++i)                             \
        {                                                                    \
            if (!value_is<double>(args[i]))                                  \
                throw std::runtime_error("min() arguments must be numbers"); \
            m = std::min(m, value_get<double>(args[i]));                     \
        }                                                                    \
                                                                             \
        push(m);                                                             \
        break;                                                               \
    }

#define NATIVE_GLOBALS_MATH_AVERAGE                                              \
    case NativeMethod::MATH_AVERAGE:                                             \
    {                                                                            \
        if (args.size() < 1)                                                     \
            throw std::runtime_error("average() expects at least 1 argument");   \
                                                                                 \
        double sum = 0;                                                          \
        for (const auto &a : args)                                               \
        {                                                                        \
            if (!value_is<double>(a))                                            \
                throw std::runtime_error("average() arguments must be numbers"); \
            sum += value_get<double>(a);                                         \
        }                                                                        \
                                                                                 \
        push(sum / args.size());                                                 \
        break;                                                                   \
    }

#define NATIVE_GLOBALS_SIN                                               \
    case NativeMethod::SIN:                                              \
    {                                                                    \
        if (args.size() != 1)                                            \
            throw std::runtime_error("sin() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                  \
            throw std::runtime_error("sin() argument must be a number"); \
        push(std::sin(value_get<double>(args[0])));                      \
        break;                                                           \
    }

#define NATIVE_GLOBALS_COS                                               \
    case NativeMethod::COS:                                              \
    {                                                                    \
        if (args.size() != 1)                                            \
            throw std::runtime_error("cos() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                  \
            throw std::runtime_error("cos() argument must be a number"); \
        push(std::cos(value_get<double>(args[0])));                      \
        break;                                                           \
    }

#define NATIVE_GLOBALS_TAN                                               \
    case NativeMethod::TAN:                                              \
    {                                                                    \
        if (args.size() != 1)                                            \
            throw std::runtime_error("tan() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                  \
            throw std::runtime_error("tan() argument must be a number"); \
        push(std::tan(value_get<double>(args[0])));                      \
        break;                                                           \
    }

#define NATIVE_GLOBALS_ASIN                                                       \
    case NativeMethod::ASIN:                                                      \
    {                                                                             \
        if (args.size() != 1)                                                     \
            throw std::runtime_error("asin() expects 1 argument");                \
        if (!value_is<double>(args[0]))                                           \
            throw std::runtime_error("asin() argument must be a number");         \
        double x = value_get<double>(args[0]);                                    \
        if (x < -1 || x > 1)                                                      \
            throw std::runtime_error("asin() domain error: x must be in [-1,1]"); \
        push(std::asin(x));                                                       \
        break;                                                                    \
    }

#define NATIVE_GLOBALS_ACOS                                                       \
    case NativeMethod::ACOS:                                                      \
    {                                                                             \
        if (args.size() != 1)                                                     \
            throw std::runtime_error("acos() expects 1 argument");                \
        if (!value_is<double>(args[0]))                                           \
            throw std::runtime_error("acos() argument must be a number");         \
        double x = value_get<double>(args[0]);                                    \
        if (x < -1 || x > 1)                                                      \
            throw std::runtime_error("acos() domain error: x must be in [-1,1]"); \
        push(std::acos(x));                                                       \
        break;                                                                    \
    }

#define NATIVE_GLOBALS_ATAN                                               \
    case NativeMethod::ATAN:                                              \
    {                                                                     \
        if (args.size() != 1)                                             \
            throw std::runtime_error("atan() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                   \
            throw std::runtime_error("atan() argument must be a number"); \
        push(std::atan(value_get<double>(args[0])));                      \
        break;                                                            \
    }

#define NATIVE_GLOBALS_DEGREES                                               \
    case NativeMethod::DEGREES:                                              \
    {                                                                        \
        if (args.size() != 1)                                                \
            throw std::runtime_error("degrees() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                      \
            throw std::runtime_error("degrees() argument must be a number"); \
        push(value_get<double>(args[0]) * 180.0 / M_PI);                     \
        break;                                                               \
    }

#define NATIVE_GLOBALS_RADIANS                                               \
    case NativeMethod::RADIANS:                                              \
    {                                                                        \
        if (args.size() != 1)                                                \
            throw std::runtime_error("radians() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                      \
            throw std::runtime_error("radians() argument must be a number"); \
        push(value_get<double>(args[0]) * M_PI / 180.0);                     \
        break;                                                               \
    }

#define NATIVE_GLOBALS_IS_ODD                                               \
    case NativeMethod::IS_ODD:                                              \
    {                                                                       \
        if (args.size() != 1)                                               \
            throw std::runtime_error("is_odd() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                     \
            throw std::runtime_error("is_odd() argument must be a number"); \
        double v = value_get<double>(args[0]);                              \
        if (std::floor(v) != v)                                             \
            throw std::runtime_error("is_odd() requires whole number");     \
        push(static_cast<int>(v) % 2 != 0 ? true : false);                  \
        break;                                                              \
    }

#define NATIVE_GLOBALS_IS_EVEN                                               \
    case NativeMethod::IS_EVEN:                                              \
    {                                                                        \
        if (args.size() != 1)                                                \
            throw std::runtime_error("is_even() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                      \
            throw std::runtime_error("is_even() argument must be a number"); \
        double v = value_get<double>(args[0]);                               \
        if (std::floor(v) != v)                                              \
            throw std::runtime_error("is_even() requires whole number");     \
        push(static_cast<int>(v) % 2 == 0 ? true : false);                   \
        break;                                                               \
    }

#define NATIVE_GLOBALS_IS_PRIME                                               \
    case NativeMethod::IS_PRIME:                                              \
    {                                                                         \
        if (args.size() != 1)                                                 \
            throw std::runtime_error("is_prime() expects 1 argument");        \
        if (!value_is<double>(args[0]))                                       \
            throw std::runtime_error("is_prime() argument must be a number"); \
                                                                              \
        double v = value_get<double>(args[0]);                                \
        if (std::floor(v) != v || v < 2)                                      \
        {                                                                     \
            push(false);                                                      \
            break;                                                            \
        }                                                                     \
        int n = static_cast<int>(v);                                          \
        bool prime = true;                                                    \
        for (int i = 2; i * i <= n; ++i)                                      \
        {                                                                     \
            if (n % i == 0)                                                   \
            {                                                                 \
                prime = false;                                                \
                break;                                                        \
            }                                                                 \
        }                                                                     \
        push(prime ? true : false);                                           \
        break;                                                                \
    }

#define NATIVE_GLOBALS_GCD                                               \
    case NativeMethod::GCD:                                              \
    {                                                                    \
        if (args.size() != 2)                                            \
            throw std::runtime_error("gcd() expects 2 arguments");       \
        if (!value_is<double>(args[0]) ||                                \
            !value_is<double>(args[1]))                                  \
            throw std::runtime_error("gcd() arguments must be numbers"); \
        int a = std::abs(static_cast<int>(value_get<double>(args[0])));  \
        int b = std::abs(static_cast<int>(value_get<double>(args[1])));  \
        while (b != 0)                                                   \
        {                                                                \
            int t = b;                                                   \
            b = a % b;                                                   \
            a = t;                                                       \
        }                                                                \
        push(static_cast<double>(a));                                    \
        break;                                                           \
    }

#define NATIVE_GLOBALS_LCM                                               \
    case NativeMethod::LCM:                                              \
    {                                                                    \
        if (args.size() != 2)                                            \
            throw std::runtime_error("lcm() expects 2 arguments");       \
        if (!value_is<double>(args[0]) ||                                \
            !value_is<double>(args[1]))                                  \
            throw std::runtime_error("lcm() arguments must be numbers"); \
        int a = static_cast<int>(value_get<double>(args[0]));            \
        int b = static_cast<int>(value_get<double>(args[1]));            \
        int g = std::gcd(a, b);                                          \
        push(static_cast<double>(std::abs(a * b) / g));                  \
        break;                                                           \
    }

#define NATIVE_GLOBALS_NEXT_PRIME                                               \
    case NativeMethod::NEXT_PRIME:                                              \
    {                                                                           \
        if (args.size() != 1)                                                   \
            throw std::runtime_error("next_prime() expects 1 argument");        \
                                                                                \
        if (!value_is<double>(args[0]))                                         \
            throw std::runtime_error("next_prime() argument must be a number"); \
                                                                                \
        double v = value_get<double>(args[0]);                                  \
        if (std::floor(v) != v)                                                 \
            throw std::runtime_error("next_prime() requires whole number");     \
                                                                                \
        int n = static_cast<int>(v);                                            \
        if (n < 2)                                                              \
            n = 1;                                                              \
                                                                                \
        auto isPrime = [](int x)                                                \
        {                                                                       \
            if (x < 2)                                                          \
                return false;                                                   \
            for (int i = 2; i * i <= x; ++i)                                    \
                if (x % i == 0)                                                 \
                    return false;                                               \
            return true;                                                        \
        };                                                                      \
                                                                                \
        int candidate = n + 1;                                                  \
        while (!isPrime(candidate))                                             \
            ++candidate;                                                        \
                                                                                \
        push(static_cast<double>(candidate));                                   \
        break;                                                                  \
    }

#define NATIVE_GLOBALS_SUM_DIGITS                                               \
    case NativeMethod::SUM_DIGITS:                                              \
    {                                                                           \
        if (args.size() != 1)                                                   \
            throw std::runtime_error("sum_digits() expects 1 argument");        \
                                                                                \
        if (!value_is<double>(args[0]))                                         \
            throw std::runtime_error("sum_digits() argument must be a number"); \
                                                                                \
        double v = value_get<double>(args[0]);                                  \
        if (std::floor(v) != v)                                                 \
            throw std::runtime_error("sum_digits() requires whole number");     \
                                                                                \
        int n = std::abs(static_cast<int>(v));                                  \
        int sum = 0;                                                            \
        while (n > 0)                                                           \
        {                                                                       \
            sum += n % 10;                                                      \
            n /= 10;                                                            \
        }                                                                       \
                                                                                \
        push(static_cast<double>(sum));                                         \
        break;                                                                  \
    }

#define NATIVE_GLOBALS_FIBONACCI                                               \
    case NativeMethod::FIBONACCI:                                              \
    {                                                                          \
        if (args.size() != 1)                                                  \
            throw std::runtime_error("fibonacci() expects 1 argument");        \
                                                                               \
        if (!value_is<double>(args[0]))                                        \
            throw std::runtime_error("fibonacci() argument must be a number"); \
                                                                               \
        double v = value_get<double>(args[0]);                                 \
        if (std::floor(v) != v)                                                \
            throw std::runtime_error("fibonacci() requires whole number");     \
                                                                               \
        int n = static_cast<int>(v);                                           \
        if (n < 0)                                                             \
            throw std::runtime_error("fibonacci() of negative number");        \
                                                                               \
        if (n == 0)                                                            \
        {                                                                      \
            push(0.0);                                                         \
            break;                                                             \
        }                                                                      \
        if (n == 1)                                                            \
        {                                                                      \
            push(1.0);                                                         \
            break;                                                             \
        }                                                                      \
                                                                               \
        double a = 0, b = 1;                                                   \
        for (int i = 2; i <= n; ++i)                                           \
        {                                                                      \
            double temp = a + b;                                               \
            a = b;                                                             \
            b = temp;                                                          \
        }                                                                      \
                                                                               \
        push(b);                                                               \
        break;                                                                 \
    }

#define NATIVE_GLOBALS_PALINDROME                                               \
    case NativeMethod::PALINDROME:                                              \
    {                                                                           \
        if (args.size() != 1)                                                   \
            throw std::runtime_error("palindrome() expects 1 argument");        \
                                                                                \
        if (!value_is<double>(args[0]))                                         \
            throw std::runtime_error("palindrome() argument must be a number"); \
                                                                                \
        double v = value_get<double>(args[0]);                                  \
        if (std::floor(v) != v)                                                 \
            throw std::runtime_error("palindrome() requires whole number");     \
                                                                                \
        int n = std::abs(static_cast<int>(v));                                  \
        int original = n;                                                       \
        int reversed = 0;                                                       \
                                                                                \
        while (n > 0)                                                           \
        {                                                                       \
            reversed = reversed * 10 + (n % 10);                                \
            n /= 10;                                                            \
        }                                                                       \
                                                                                \
        push(original == reversed ? true : false);                              \
        break;                                                                  \
    }

#define NATIVE_GLOBALS_ROUND                                               \
    case NativeMethod::ROUND:                                              \
    {                                                                      \
        if (args.size() != 1)                                              \
            throw std::runtime_error("round() expects 1 argument");        \
                                                                           \
        if (!value_is<double>(args[0]))                                    \
            throw std::runtime_error("round() argument must be a number"); \
                                                                           \
        double x = value_get<double>(args[0]);                             \
        double result;                                                     \
        if (x >= 0.0)                                                      \
            result = std::floor(x + 0.5);                                  \
        else                                                               \
            result = std::ceil(x - 0.5);                                   \
                                                                           \
        push(result);                                                      \
        break;                                                             \
    }

#define NATIVE_GLOBALS_PI                                         \
    case NativeMethod::PI:                                        \
    {                                                             \
        if (!args.empty())                                        \
            throw std::runtime_error("pi() expects 0 arguments"); \
        push(M_PI);                                               \
        break;                                                    \
    }

#define NATIVE_GLOBALS_PRECISION                                                           \
    case NativeMethod::PRECISION:                                                          \
    {                                                                                      \
        if (args.size() != 2)                                                              \
            throw std::runtime_error("precision() expects 2 arguments: value and digits"); \
                                                                                           \
        if (!value_is<double>(args[0]) ||                                                  \
            !value_is<double>(args[1]))                                                    \
            throw std::runtime_error("precision() arguments must be numbers");             \
                                                                                           \
        double value = value_get<double>(args[0]);                                         \
        double digits_d = value_get<double>(args[1]);                                      \
        int digits = static_cast<int>(digits_d);                                           \
        if (digits < 0)                                                                    \
            throw std::runtime_error("precision() digits must be non-negative");           \
                                                                                           \
        double factor = std::pow(10.0, digits);                                            \
        double result = std::floor(value * factor) / factor;                               \
        push(result);                                                                      \
        break;                                                                             \
    }
