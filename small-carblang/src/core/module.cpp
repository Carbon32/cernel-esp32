#include "core/core.hpp"

std::shared_ptr<BoundMethod> make_native_method(Value value, NativeMethod method)
{
    return std::make_shared<BoundMethod>(std::move(value), method);
}

std::shared_ptr<Instance> create_module_instance(Module &&module, const std::string &name)
{
    auto klass = std::make_shared<Class>(name, std::unordered_map<std::string, std::shared_ptr<Function>>{});
    auto instance = std::make_shared<Instance>(klass);

    for (auto &[method_name, method_value] : module.methods)
    {
        instance->fields[method_name] = method_value;
    }

    return instance;
}

std::shared_ptr<Instance> init_regex()
{
    Module regex;
    regex.set_method("match", make_native_method(nullptr, NativeMethod::MATCH));
    return create_module_instance(std::move(regex), "regex");
}

std::shared_ptr<Instance> init_time()
{
    Module time;
    time.set_method("date", make_native_method(nullptr, NativeMethod::DATE));
    time.set_method("second_to_minute", make_native_method(nullptr, NativeMethod::SECOND_TO_MINUTE));
    time.set_method("second_to_hour", make_native_method(nullptr, NativeMethod::SECOND_TO_HOUR));
    time.set_method("second_to_day", make_native_method(nullptr, NativeMethod::SECOND_TO_DAY));
    time.set_method("minute_to_second", make_native_method(nullptr, NativeMethod::MINUTE_TO_SECOND));
    time.set_method("minute_to_hour", make_native_method(nullptr, NativeMethod::MINUTE_TO_HOUR));
    time.set_method("minute_to_day", make_native_method(nullptr, NativeMethod::MINUTE_TO_DAY));
    time.set_method("hour_to_second", make_native_method(nullptr, NativeMethod::HOUR_TO_SECOND));
    time.set_method("hour_to_minute", make_native_method(nullptr, NativeMethod::HOUR_TO_MINUTE));
    time.set_method("hour_to_day", make_native_method(nullptr, NativeMethod::HOUR_TO_DAY));
    time.set_method("leap_year", make_native_method(nullptr, NativeMethod::LEAP_YEAR));
    time.set_method("today", make_native_method(nullptr, NativeMethod::TODAY));
    return create_module_instance(std::move(time), "timing");
}

std::shared_ptr<Instance> init_encode()
{
    Module encode;
    encode.set_method("base", make_native_method(nullptr, NativeMethod::BASE));
    return create_module_instance(std::move(encode), "encode");
}

std::shared_ptr<Instance> init_math()
{
    Module math;
    math.set_method("pow", make_native_method(nullptr, NativeMethod::POW));
    math.set_method("sqrt", make_native_method(nullptr, NativeMethod::SQRT));
    math.set_method("fact", make_native_method(nullptr, NativeMethod::FACT));
    math.set_method("floor", make_native_method(nullptr, NativeMethod::FLOOR));
    math.set_method("ceil", make_native_method(nullptr, NativeMethod::CEIL));
    math.set_method("abs", make_native_method(nullptr, NativeMethod::ABS));
    math.set_method("log", make_native_method(nullptr, NativeMethod::LOG));
    math.set_method("log10", make_native_method(nullptr, NativeMethod::LOG10));
    math.set_method("log2", make_native_method(nullptr, NativeMethod::LOG2));
    math.set_method("max", make_native_method(nullptr, NativeMethod::MATH_MAX));
    math.set_method("min", make_native_method(nullptr, NativeMethod::MATH_MIN));
    math.set_method("average", make_native_method(nullptr, NativeMethod::MATH_AVERAGE));
    math.set_method("sin", make_native_method(nullptr, NativeMethod::SIN));
    math.set_method("cos", make_native_method(nullptr, NativeMethod::COS));
    math.set_method("tan", make_native_method(nullptr, NativeMethod::TAN));
    math.set_method("asin", make_native_method(nullptr, NativeMethod::ASIN));
    math.set_method("acos", make_native_method(nullptr, NativeMethod::ACOS));
    math.set_method("atan", make_native_method(nullptr, NativeMethod::ATAN));
    math.set_method("degrees", make_native_method(nullptr, NativeMethod::DEGREES));
    math.set_method("radians", make_native_method(nullptr, NativeMethod::RADIANS));
    math.set_method("is_odd", make_native_method(nullptr, NativeMethod::IS_ODD));
    math.set_method("is_even", make_native_method(nullptr, NativeMethod::IS_EVEN));
    math.set_method("is_prime", make_native_method(nullptr, NativeMethod::IS_PRIME));
    math.set_method("gcd", make_native_method(nullptr, NativeMethod::GCD));
    math.set_method("lcm", make_native_method(nullptr, NativeMethod::LCM));
    math.set_method("next_prime", make_native_method(nullptr, NativeMethod::NEXT_PRIME));
    math.set_method("sum_digits", make_native_method(nullptr, NativeMethod::SUM_DIGITS));
    math.set_method("fibonacci", make_native_method(nullptr, NativeMethod::FIBONACCI));
    math.set_method("palindrome", make_native_method(nullptr, NativeMethod::PALINDROME));
    math.set_method("round", make_native_method(nullptr, NativeMethod::ROUND));
    math.set_method("pi", make_native_method(nullptr, NativeMethod::PI));
    math.set_method("precision", make_native_method(nullptr, NativeMethod::PRECISION));
    return create_module_instance(std::move(math), "math");
}

std::shared_ptr<Instance> init_text()
{
    Module text;
    text.set_method("parse_json", make_native_method(nullptr, NativeMethod::PARSE_JSON));
    text.set_method("stringify", make_native_method(nullptr, NativeMethod::STRINGIFY));
    text.set_method("to_json", make_native_method(nullptr, NativeMethod::TO_JSON));
    text.set_method("parse_csv", make_native_method(nullptr, NativeMethod::PARSE_CSV));
    text.set_method("to_csv", make_native_method(nullptr, NativeMethod::TO_CSV));
    return create_module_instance(std::move(text), "text");
}

std::shared_ptr<Instance> init_random()
{
    Module random;
    random.set_method("double", make_native_method(nullptr, NativeMethod::RAND));
    random.set_method("integer", make_native_method(nullptr, NativeMethod::RANDINT));
    random.set_method("rgb_color", make_native_method(nullptr, NativeMethod::RANDOM_COLOR_RGB));
    random.set_method("hex_color", make_native_method(nullptr, NativeMethod::RANDOM_COLOR_HEX));
    return create_module_instance(std::move(random), "random");
}

std::shared_ptr<Instance> init_array()
{
    Module array;
    array.set_method("fill", make_native_method(nullptr, NativeMethod::FILL));
    array.set_method("init", make_native_method(nullptr, NativeMethod::INIT));
    array.set_method("input", make_native_method(nullptr, NativeMethod::ARRAY_INPUT));
    return create_module_instance(std::move(array), "array");
}

std::shared_ptr<Instance> init_profiler()
{
    Module profiler;
    profiler.set_method("start", make_native_method(nullptr, NativeMethod::PROFILE_START));
    profiler.set_method("end", make_native_method(nullptr, NativeMethod::PROFILE_END));
    profiler.set_method("report", make_native_method(nullptr, NativeMethod::PROFILE_REPORT));
    profiler.set_method("reset", make_native_method(nullptr, NativeMethod::PROFILE_RESET));
    return create_module_instance(std::move(profiler), "profiler");
}

