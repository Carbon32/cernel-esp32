#include "core/core.hpp"

#ifdef ESP_PLATFORM
extern "C" void carblang_uart_write(const char *data, size_t size);
extern "C" void carblang_led_on(void);
extern "C" void carblang_led_off(void);
extern "C" void carblang_led_toggle(void);
#endif

bool compare_values(const Value &a, const Value &b, TokenType op)
{
    if (!value_is<double>(a) || !value_is<double>(b))
        throw std::runtime_error("Comparison operators <, <=, >, >= can only be used with numbers");

    double x = value_get<double>(a);
    double y = value_get<double>(b);

    switch (op)
    {
    case GREATER:
        return x > y;
    case GREATER_EQUAL:
        return x >= y;
    case LESS:
        return x < y;
    case LESS_EQUAL:
        return x <= y;
    default:
        throw std::runtime_error("Invalid numeric comparison operator");
    }
}

static size_t checked_sequence_index(const Value &value, size_t length, const char *kind)
{
    const auto *number = value_get_if<double>(&value);
    if (!number || !std::isfinite(*number))
        throw std::runtime_error(std::string(kind) + " index must be a finite number");

    const double index = std::trunc(*number);
    if (index < 0 || index >= static_cast<double>(length))
        throw std::runtime_error(std::string(kind) + " index out of bounds");

    return static_cast<size_t>(index);
}

static size_t checked_size_count(const Value &value, size_t maximum, const char *kind)
{
    const auto *number = value_get_if<double>(&value);
    if (!number || !std::isfinite(*number))
        throw std::runtime_error(std::string(kind) + " must be a finite number");
    const double count = std::trunc(*number);
    if (count < 0 || static_cast<long double>(count) > static_cast<long double>(maximum))
        throw std::runtime_error(std::string(kind) + " is out of range");
    return static_cast<size_t>(count);
}

static size_t checked_combined_size(size_t left, size_t right, size_t maximum, const char *kind)
{
    if (right > maximum - left)
        throw std::runtime_error(std::string(kind) + " is too large");
    return left + right;
}

static std::vector<std::string> runtime_type_arguments(const std::string &type)
{
    const size_t open = type.find('[');
    if (open == std::string::npos || type.back() != ']')
        return {};

    std::vector<std::string> arguments;
    size_t start = open + 1;
    int depth = 0;
    for (size_t i = start; i < type.size() - 1; ++i)
    {
        if (type[i] == '[')
            ++depth;
        else if (type[i] == ']')
            --depth;
        else if (type[i] == ',' && depth == 0)
        {
            arguments.push_back(type.substr(start, i - start));
            start = i + 1;
        }
    }
    arguments.push_back(type.substr(start, type.size() - 1 - start));
    return arguments;
}

static bool runtime_type_matches(const Value &value, const std::string &type)
{
    if (type == "any" || type == "mixed")
        return true;

    const size_t open = type.find('[');
    const std::string base = open == std::string::npos ? type : type.substr(0, open);
    const auto arguments = runtime_type_arguments(type);
    if (base == "number") return value_is<double>(value);
    if (base == "string") return value_is<std::string>(value);
    if (base == "bool") return value_is<bool>(value);
    if (base == "null" || base == "nothing") return value_is<std::nullptr_t>(value);
    if (base == "function") return value_is<std::shared_ptr<Function>>(value);
    if (base == "class") return value_is<std::shared_ptr<Class>>(value);
    if (base == "bound_method") return value_is<std::shared_ptr<UserBoundMethod>>(value);
    if (base == "native_method") return value_is<std::shared_ptr<BoundMethod>>(value);

    if (base == "array")
    {
        const auto *array = value_get_if<std::shared_ptr<Array>>(&value);
        if (!array || !*array)
            return false;
        if (arguments.size() == 1)
            for (const auto &element : (*array)->elements)
                if (!runtime_type_matches(element, arguments.front()))
                    return false;
        return true;
    }

    if (base == "dict")
    {
        const auto *dict = value_get_if<std::shared_ptr<Dict>>(&value);
        if (!dict || !*dict)
            return false;
        if (arguments.size() == 2)
            for (const auto &[key, element] : (*dict)->entries)
                if (!runtime_type_matches(Value{key}, arguments.front()) ||
                    !runtime_type_matches(element, arguments.back()))
                    return false;
        return true;
    }

    const auto *instance = value_get_if<std::shared_ptr<Instance>>(&value);
    if (!instance || !*instance)
        return false;
    for (auto klass = (*instance)->klass; klass; klass = klass->super_class)
        if (klass->name == base)
            return true;
    return false;
}

static size_t clamped_sequence_index(const Value &value, size_t length, const char *kind)
{
    const auto *number = value_get_if<double>(&value);
    if (!number || !std::isfinite(*number))
        throw std::runtime_error(std::string(kind) + " must be a finite number");
    const double index = std::trunc(*number);
    if (index <= 0)
        return 0;
    if (index >= static_cast<double>(length))
        return length;
    return static_cast<size_t>(index);
}

static std::string repeat_string(const std::string &value, double count)
{
    if (!std::isfinite(count) || std::floor(count) != count)
        throw std::runtime_error("Cannot multiply string by non-integer number");
    if (count <= 0 || value.empty())
        return {};

    std::string result;
    const size_t max_repetitions = result.max_size() / value.size();
    if (static_cast<long double>(count) > static_cast<long double>(max_repetitions))
        throw std::runtime_error("String repetition result is too large");

    const size_t repetitions = static_cast<size_t>(count);
    result.reserve(value.size() * repetitions);
    for (size_t i = 0; i < repetitions; ++i)
        result.append(value);
    return result;
}

void VM::interpret(Chunk &new_chunk)
{
    global_cache.clear();
    method_cache.clear();
    chunk = &new_chunk;
    ip = chunk->code.data();
    stack.clear();
    stack.reserve(std::min<size_t>(new_chunk.code.size() / 8 + 16, 1024));
    frames.clear();
    frames.reserve(16);
    native_args.reserve(8);
    run();
}

void VM::init_globals(bool interactive_mode)
{
    globals.reserve(64); const_variables.reserve(16);
    globals["math"] = init_math(); globals["regex"] = init_regex(); globals["timing"] = init_time();
    globals["encode"] = init_encode(); globals["text"] = init_text(); globals["random"] = init_random();
    globals["array"] = init_array(); globals["profiler"] = init_profiler();
    for (const auto *name : {"math", "regex", "timing", "encode", "text", "random", "array", "profiler", "scan", "format", "println", "methods"})
        const_variables.insert(name);
    globals["scan"] = make_native_method(nullptr, NativeMethod::INPUT);
    globals["format"] = make_native_method(nullptr, NativeMethod::FORMAT);
    globals["println"] = make_native_method(nullptr, NativeMethod::PRINTLN);
    globals["methods"] = make_native_method(nullptr, NativeMethod::INSTANCE_METHODS);
    for (const auto *name : {"led_on", "led_off", "led_toggle"})
        const_variables.insert(name);
    globals["led_on"] = make_native_method(nullptr, NativeMethod::LED_ON);
    globals["led_off"] = make_native_method(nullptr, NativeMethod::LED_OFF);
    globals["led_toggle"] = make_native_method(nullptr, NativeMethod::LED_TOGGLE);

    if (!interactive_mode)
    {
        const_variables.insert("print");
        const_variables.insert("printf");
        globals["print"] = make_native_method(nullptr, NativeMethod::PRINT);
        globals["printf"] = make_native_method(nullptr, NativeMethod::PRINTF);
    }
}

void VM::run()
{

    for (;;)
    {
        const uint8_t *instruction = ip;
        OpCode opcode = static_cast<OpCode>(read_byte());

        // I use this to track opcode executions, I will leave it here for now
        /* std::cout << "[DEBUG] Executing opcode: " << static_cast<int>(opcode) << ", stack: [";
        for (size_t i = 0; i < stack.size(); ++i)
        {
            std::cout << stringify(stack[i]);
            if (i + 1 < stack.size())
                std::cout << ", ";
        }
        std::cout << "]" << std::endl; */

        switch (opcode)
        {
        case OpCode::CONSTANT:
        {
            stack.push_back(chunk->constants.at(read_short()));
            break;
        }

        case OpCode::CONSTANT_SHORT:
        {
            stack.push_back(chunk->constants.at(read_byte()));
            break;
        }

        case OpCode::ADD:
        case OpCode::SUBTRACT:
        case OpCode::MULTIPLY:
        case OpCode::DIVIDE:
        case OpCode::POWER:
        {
            Value b = pop();
            Value a = pop();

            if (const double *x = value_get_if<double>(&a))
            {
                if (const double *y = value_get_if<double>(&b))
                {
                    switch (opcode)
                    {
                    case OpCode::ADD:
                        push(*x + *y);
                        break;
                    case OpCode::SUBTRACT:
                        push(*x - *y);
                        break;
                    case OpCode::MULTIPLY:
                        push(*x * *y);
                        break;
                    case OpCode::POWER:
                        push(std::pow(*x, *y));
                        break;
                    case OpCode::DIVIDE:
                        if (*y == 0)
                            throw std::runtime_error("Division by zero");
                        push(*x / *y);
                        break;
                    default:
                        break;
                    }
                    break;
                }
            }

            if (opcode == OpCode::ADD &&
                (value_is<std::string>(a) ||
                 value_is<std::string>(b)))
            {
                stack.push_back(stringify(a) + stringify(b));
                break;
            }

            if (opcode == OpCode::ADD &&
                value_is<std::shared_ptr<Array>>(a) &&
                value_is<std::shared_ptr<Array>>(b))
            {
                auto left = value_get<std::shared_ptr<Array>>(a);
                auto right = value_get<std::shared_ptr<Array>>(b);

                auto result = std::make_shared<Array>();
                const size_t combined_size = checked_combined_size(
                    left->elements.size(), right->elements.size(), result->elements.max_size(), "Array concatenation");
                result->elements.reserve(combined_size);

                result->elements.insert(
                    result->elements.end(),
                    left->elements.begin(),
                    left->elements.end());

                result->elements.insert(
                    result->elements.end(),
                    right->elements.begin(),
                    right->elements.end());

                stack.push_back(result);
                break;
            }

            if (value_is<std::string>(a) && value_is<double>(b))
            {
                if (opcode == OpCode::MULTIPLY)
                {
                    stack.push_back(repeat_string(value_get<std::string>(a), value_get<double>(b)));
                    break;
                }
            }

            if (value_is<double>(a) && value_is<std::string>(b))
            {
                if (opcode == OpCode::MULTIPLY)
                {
                    stack.push_back(repeat_string(value_get<std::string>(b), value_get<double>(a)));
                    break;
                }
            }

            if (value_is<std::shared_ptr<Array>>(a) && value_is<double>(b))
            {
                auto arr = value_get<std::shared_ptr<Array>>(a);
                double scalar = value_get<double>(b);

                auto result = std::make_shared<Array>();
                result->elements.reserve(arr->elements.size());

                for (const auto &v : arr->elements)
                {
                    if (!value_is<double>(v))
                        throw std::runtime_error("Array must contain only numbers");

                    double x = value_get<double>(v);
                    double r;

                    switch (opcode)
                    {
                    case OpCode::ADD:
                        r = x + scalar;
                        break;
                    case OpCode::SUBTRACT:
                        r = x - scalar;
                        break;
                    case OpCode::MULTIPLY:
                        r = x * scalar;
                        break;
                    case OpCode::DIVIDE:
                        if (scalar == 0)
                            throw std::runtime_error("Division by zero");
                        r = x / scalar;
                        break;
                    default:
                        r = 0;
                    }

                    result->elements.push_back(r);
                }

                stack.push_back(result);
                break;
            }

            throw std::runtime_error("Operands must be numbers: " + stringify(a) + " " + stringify(b));
        }

        case OpCode::PERCENT:
        {
            Value b = pop();
            Value a = pop();

            if (value_is<double>(a) && value_is<double>(b))
            {
                double x = value_get<double>(a);
                double y = value_get<double>(b);
                if (y == 0.0)
                {
                    throw std::runtime_error("Division by zero");
                }
                stack.push_back(std::fmod(x, y));
            }

            else
            {
                throw std::runtime_error("Operands must be numbers");
            }
            break;
        }

        case OpCode::NOT:
        {
            Value v = pop();
            stack.push_back(!is_truthy(v));
            break;
        }

        case OpCode::NEGATE:
        {
            Value v = pop();
            if (!value_is<double>(v))
                throw std::runtime_error("Operand must be a number");
            stack.push_back(-value_get<double>(v));
            break;
        }

        case OpCode::EQUAL:
        {
            Value b = pop();
            Value a = pop();
            stack.push_back(a == b);
            break;
        }

        case OpCode::GREATER_EQUAL:
        {
            auto b = pop();
            auto a = pop();
            push(compare_values(a, b, GREATER_EQUAL));
            break;
        }

        case OpCode::LESS_EQUAL:
        {
            auto b = pop();
            auto a = pop();
            push(compare_values(a, b, LESS_EQUAL));
            break;
        }

        case OpCode::NOT_EQUAL:
        {
            auto b = pop();
            auto a = pop();
            push(a != b);
            break;
        }

        case OpCode::GREATER:
        case OpCode::LESS:
        {
            Value b = pop();
            Value a = pop();
            if (!value_is<double>(a) || !value_is<double>(b))
                throw std::runtime_error("Operands must be numbers");

            double x = value_get<double>(a);
            double y = value_get<double>(b);

            stack.push_back(opcode == OpCode::GREATER ? x > y : x < y);
            break;
        }

        case OpCode::POP:
        {
            Value v = pop();

            // I use this to track how pop works, leave it here for now
            // std::cout << "[DEBUG] POP -> " << stringify(v) << std::endl;
            break;
        }

        case OpCode::DEFINE_CONST:
        {
            uint16_t name_index = read_short();
            const std::string &name =
                value_get<std::string>(chunk->constants.at(name_index));

            if (globals.contains(name))
                throw std::runtime_error("Cannot redefine constant \"" + name + "\"");

            globals[name] = pop();
            global_cache.clear();
            const_variables.insert(name);
            break;
        }

        case OpCode::DEFINE_GLOBAL:
        {
            uint16_t name_index = read_short();
            const std::string &name =
                value_get<std::string>(chunk->constants.at(name_index));

            if (const_variables.contains(name))
                throw std::runtime_error("Cannot redefine constant \"" + name + "\"");

            globals[name] = pop();
            global_cache.clear();
            break;
        }

        case OpCode::GET_GLOBAL:
        {
            uint16_t name_index = read_short();
            auto cached = global_cache.find(instruction);
            if (cached != global_cache.end())
            {
                stack.push_back(*cached->second.value);
                break;
            }

            const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
            auto it = globals.find(name);
            if (it == globals.end())
                throw std::runtime_error("Undefined variable \"" + name + "\"");

            global_cache.emplace(instruction, GlobalCacheEntry{&it->second, const_variables.contains(name)});
            stack.push_back(it->second);
            break;
        }

        case OpCode::SET_GLOBAL:
        {
            uint16_t name_index = read_short();
            auto cached = global_cache.find(instruction);
            if (cached != global_cache.end())
            {
                if (cached->second.is_const)
                {
                    const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
                    throw std::runtime_error("Cannot reassign constant \"" + name + "\"");
                }

                *cached->second.value = stack.back();
                break;
            }

            const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
            auto it = globals.find(name);
            if (it == globals.end())
                throw std::runtime_error("Undefined variable \"" + name + "\"");

            const bool is_const = const_variables.contains(name);
            if (is_const)
                throw std::runtime_error("Cannot reassign constant \"" + name + "\"");

            global_cache.emplace(instruction, GlobalCacheEntry{&it->second, is_const});
            it->second = stack.back();
            break;
        }

        case OpCode::INCREMENT_GLOBAL:
        {
            uint16_t name_index = read_short();
            const bool keep_result = read_byte() != 0;
            auto cached = global_cache.find(instruction);
            Value *target = nullptr;
            bool is_const = false;

            if (cached != global_cache.end())
            {
                target = cached->second.value;
                is_const = cached->second.is_const;
            }
            else
            {
                const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
                auto it = globals.find(name);
                if (it == globals.end())
                    throw std::runtime_error("Undefined variable \"" + name + "\"");

                is_const = const_variables.contains(name);
                target = &it->second;
                global_cache.emplace(instruction, GlobalCacheEntry{target, is_const});
            }

            if (is_const)
            {
                const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
                throw std::runtime_error("Cannot reassign constant \"" + name + "\"");
            }

            Value updated = add_one(*target);
            *target = updated;
            if (keep_result)
                push(std::move(updated));
            break;
        }

        case OpCode::INCREMENT_LOCAL:
        {
            const uint8_t slot = read_byte();
            const bool keep_result = read_byte() != 0;
            if (frames.empty())
                throw std::runtime_error("INCREMENT_LOCAL called with no active frame");

            const size_t index = frames.back().stack_start + slot;
            if (index >= stack.size())
                throw std::runtime_error("INCREMENT_LOCAL out of bounds");

            Value updated = add_one(stack[index]);
            stack[index] = updated;
            if (keep_result)
                push(std::move(updated));
            break;
        }

        case OpCode::UPDATE_GLOBAL:
        {
            const uint16_t name_index = read_short();
            const double delta = read_byte() == 1 ? 1.0 : -1.0;
            const uint8_t result_mode = read_byte();
            auto cached = global_cache.find(instruction);
            Value *target = nullptr;
            bool is_const = false;

            if (cached != global_cache.end())
            {
                target = cached->second.value;
                is_const = cached->second.is_const;
            }
            else
            {
                const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
                auto it = globals.find(name);
                if (it == globals.end())
                    throw std::runtime_error("Undefined variable \"" + name + "\"");
                target = &it->second;
                is_const = const_variables.contains(name);
                global_cache.emplace(instruction, GlobalCacheEntry{target, is_const});
            }

            if (is_const)
            {
                const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
                throw std::runtime_error("Cannot reassign constant \"" + name + "\"");
            }

            const double *number = value_get_if<double>(target);
            if (!number)
                throw std::runtime_error("Increment and decrement require a number");
            const double previous = *number;
            *target = previous + delta;

            if (result_mode == 1)
                push(*target);
            else if (result_mode == 2)
                push(previous);
            else if (result_mode != 0)
                throw std::runtime_error("Invalid increment/decrement result mode");
            break;
        }

        case OpCode::UPDATE_LOCAL:
        {
            const uint8_t slot = read_byte();
            const double delta = read_byte() == 1 ? 1.0 : -1.0;
            const uint8_t result_mode = read_byte();
            if (frames.empty())
                throw std::runtime_error("UPDATE_LOCAL called with no active frame");

            const size_t index = frames.back().stack_start + slot;
            if (index >= stack.size())
                throw std::runtime_error("UPDATE_LOCAL out of bounds");

            const double *number = value_get_if<double>(&stack[index]);
            if (!number)
                throw std::runtime_error("Increment and decrement require a number");
            const double previous = *number;
            stack[index] = previous + delta;

            if (result_mode == 1)
                push(stack[index]);
            else if (result_mode == 2)
                push(previous);
            else if (result_mode != 0)
                throw std::runtime_error("Invalid increment/decrement result mode");
            break;
        }

        case OpCode::UPDATE_PROPERTY:
        {
            const uint16_t name_index = read_short();
            const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
            const double delta = read_byte() == 1 ? 1.0 : -1.0;
            const uint8_t result_mode = read_byte();
            Value object = pop();
            auto instance = value_get_if<std::shared_ptr<Instance>>(&object);
            if (!instance || !*instance)
                throw std::runtime_error("Only instance fields can be incremented or decremented");

            auto field = (*instance)->fields.find(name);
            if (field == (*instance)->fields.end())
                throw std::runtime_error("Undefined property \"" + name + "\"");
            const double *number = value_get_if<double>(&field->second);
            if (!number)
                throw std::runtime_error("Increment and decrement require a number");
            const double previous = *number;
            field->second = previous + delta;

            if (result_mode == 1)
                push(field->second);
            else if (result_mode == 2)
                push(previous);
            else if (result_mode != 0)
                throw std::runtime_error("Invalid increment/decrement result mode");
            break;
        }

        case OpCode::UPDATE_INDEX:
        {
            const double delta = read_byte() == 1 ? 1.0 : -1.0;
            const uint8_t result_mode = read_byte();
            Value index_value = pop();
            Value container_value = pop();
            double previous;
            double updated;

            if (auto array = value_get_if<std::shared_ptr<Array>>(&container_value))
            {
                if (!*array)
                    throw std::runtime_error("Cannot increment an uninitialized array");
                const size_t index = checked_sequence_index(index_value, (*array)->elements.size(), "Array");
                const double *number = value_get_if<double>(&(*array)->elements[index]);
                if (!number)
                    throw std::runtime_error("Increment and decrement require a number");
                previous = *number;
                updated = previous + delta;
                (*array)->elements[index] = updated;
            }
            else if (auto dict = value_get_if<std::shared_ptr<Dict>>(&container_value))
            {
                if (!*dict)
                    throw std::runtime_error("Cannot increment an uninitialized dictionary");
                if (!value_is<std::string>(index_value))
                    throw std::runtime_error("Dictionary key must be string");
                const std::string &key = value_get<std::string>(index_value);
                Value current = (*dict)->get(key);
                const double *number = value_get_if<double>(&current);
                if (!number)
                    throw std::runtime_error("Increment and decrement require a number");
                previous = *number;
                updated = previous + delta;
                (*dict)->set(key, updated);
            }
            else
                throw std::runtime_error("Only array and dictionary elements can be incremented or decremented");

            if (result_mode == 1)
                push(updated);
            else if (result_mode == 2)
                push(previous);
            else if (result_mode != 0)
                throw std::runtime_error("Invalid increment/decrement result mode");
            break;
        }

        case OpCode::JUMP_IF_GLOBAL_COMPARE:
        {
            const uint16_t name_index = read_short();
            const uint16_t limit_index = read_short();
            const TokenType comparison = static_cast<TokenType>(read_byte());
            const uint16_t offset = read_short();

            auto cached = global_cache.find(instruction);
            Value *target = nullptr;
            if (cached != global_cache.end())
            {
                target = cached->second.value;
            }
            else
            {
                const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
                auto it = globals.find(name);
                if (it == globals.end())
                    throw std::runtime_error("Undefined variable \"" + name + "\"");

                target = &it->second;
                global_cache.emplace(instruction, GlobalCacheEntry{target, const_variables.contains(name)});
            }

            if (!compare_values(*target, chunk->constants.at(limit_index), comparison))
                ip += offset;
            break;
        }

        case OpCode::JUMP_IF_LOCAL_COMPARE:
        {
            const uint8_t slot = read_byte();
            const uint16_t limit_index = read_short();
            const TokenType comparison = static_cast<TokenType>(read_byte());
            const uint16_t offset = read_short();

            if (frames.empty())
                throw std::runtime_error("JUMP_IF_LOCAL_COMPARE called with no active frame");

            const size_t index = frames.back().stack_start + slot;
            if (index >= stack.size())
                throw std::runtime_error("JUMP_IF_LOCAL_COMPARE out of bounds");

            if (!compare_values(stack[index], chunk->constants.at(limit_index), comparison))
                ip += offset;
            break;
        }

        case OpCode::LOOP_GLOBAL_INCREMENT_COMPARE:
        {
            const uint16_t name_index = read_short();
            const uint16_t limit_index = read_short();
            const TokenType comparison = static_cast<TokenType>(read_byte());
            const uint8_t update_mode = read_byte();
            const uint16_t offset = read_short();

            auto cached = global_cache.find(instruction);
            Value *target = nullptr;
            bool is_const = false;
            if (cached != global_cache.end())
            {
                target = cached->second.value;
                is_const = cached->second.is_const;
            }
            else
            {
                const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
                auto it = globals.find(name);
                if (it == globals.end())
                    throw std::runtime_error("Undefined variable \"" + name + "\"");
                target = &it->second;
                is_const = const_variables.contains(name);
                global_cache.emplace(instruction, GlobalCacheEntry{target, is_const});
            }

            if (is_const)
                throw std::runtime_error("Cannot reassign constant \"" +
                                         value_get<std::string>(chunk->constants.at(name_index)) + "\"");

            if (update_mode == 0)
                *target = add_one(*target);
            else
            {
                const double *number = value_get_if<double>(target);
                if (!number)
                    throw std::runtime_error("Increment and decrement require a number");
                *target = *number + (update_mode == 1 ? 1.0 : -1.0);
            }
            if (compare_values(*target, chunk->constants.at(limit_index), comparison))
                ip -= offset;
            break;
        }

        case OpCode::LOOP_LOCAL_INCREMENT_COMPARE:
        {
            const uint8_t slot = read_byte();
            const uint16_t limit_index = read_short();
            const TokenType comparison = static_cast<TokenType>(read_byte());
            const uint8_t update_mode = read_byte();
            const uint16_t offset = read_short();

            if (frames.empty())
                throw std::runtime_error("LOOP_LOCAL_INCREMENT_COMPARE called with no active frame");
            const size_t index = frames.back().stack_start + slot;
            if (index >= stack.size())
                throw std::runtime_error("LOOP_LOCAL_INCREMENT_COMPARE out of bounds");

            if (update_mode == 0)
                stack[index] = add_one(stack[index]);
            else
            {
                const double *number = value_get_if<double>(&stack[index]);
                if (!number)
                    throw std::runtime_error("Increment and decrement require a number");
                stack[index] = *number + (update_mode == 1 ? 1.0 : -1.0);
            }
            if (compare_values(stack[index], chunk->constants.at(limit_index), comparison))
                ip -= offset;
            break;
        }

        case OpCode::FAST_FORWARD_GLOBAL_LOOP:
        {
            const uint16_t name_index = read_short();
            const uint16_t limit_index = read_short();
            const TokenType comparison = static_cast<TokenType>(read_byte());
            const uint8_t update_mode = read_byte();

            const std::string &name = value_get<std::string>(chunk->constants.at(name_index));
            auto it = globals.find(name);
            if (it == globals.end())
                throw std::runtime_error("Undefined variable \"" + name + "\"");
            Value &target = it->second;
            const Value &limit = chunk->constants.at(limit_index);
            const double *number = value_get_if<double>(&target);
            if (!number)
            {
                if (!compare_values(target, limit, comparison))
                    break;
                throw std::runtime_error("Increment and decrement require a number");
            }

            const double initial = *number;
            if (!compare_values(target, limit, comparison))
                break;
            if (const_variables.contains(name))
                throw std::runtime_error("Cannot reassign constant \"" + name + "\"");

            const double bound = value_get<double>(limit);
            const double direction = update_mode == 1 ? 1.0 : -1.0;
            constexpr double max_exact_integer = 9007199254740992.0;
            double count = -1.0;
            if (std::isfinite(initial) && std::isfinite(bound) &&
                std::trunc(initial) == initial && std::trunc(bound) == bound &&
                std::abs(initial) <= max_exact_integer && std::abs(bound) <= max_exact_integer)
            {
                if (update_mode == 1 && comparison == LESS)
                    count = bound - initial;
                else if (update_mode == 1 && comparison == LESS_EQUAL)
                    count = bound - initial + 1.0;
                else if (update_mode == 2 && comparison == GREATER)
                    count = initial - bound;
                else if (update_mode == 2 && comparison == GREATER_EQUAL)
                    count = initial - bound + 1.0;
            }

            if (count >= 0.0 && std::isfinite(count))
            {
                const double result = initial + direction * count;
                if (std::isfinite(result) && std::abs(result) <= max_exact_integer &&
                    !compare_values(Value(result), limit, comparison))
                {
                    target = result;
                    break;
                }
            }

            double current = initial;
            while (compare_values(Value(current), limit, comparison))
                current += direction;
            target = current;
            break;
        }

        case OpCode::FAST_FORWARD_LOCAL_LOOP:
        {
            const uint8_t slot = read_byte();
            const uint16_t limit_index = read_short();
            const TokenType comparison = static_cast<TokenType>(read_byte());
            const uint8_t update_mode = read_byte();

            if (frames.empty())
                throw std::runtime_error("FAST_FORWARD_LOCAL_LOOP called with no active frame");
            const size_t index = frames.back().stack_start + slot;
            if (index >= stack.size())
                throw std::runtime_error("FAST_FORWARD_LOCAL_LOOP out of bounds");

            Value &target = stack[index];
            const Value &limit = chunk->constants.at(limit_index);
            const double *number = value_get_if<double>(&target);
            if (!number)
            {
                if (!compare_values(target, limit, comparison))
                    break;
                throw std::runtime_error("Increment and decrement require a number");
            }

            const double initial = *number;
            if (!compare_values(target, limit, comparison))
                break;

            const double bound = value_get<double>(limit);
            const double direction = update_mode == 1 ? 1.0 : -1.0;
            constexpr double max_exact_integer = 9007199254740992.0;
            double count = -1.0;
            if (std::isfinite(initial) && std::isfinite(bound) &&
                std::trunc(initial) == initial && std::trunc(bound) == bound &&
                std::abs(initial) <= max_exact_integer && std::abs(bound) <= max_exact_integer)
            {
                if (update_mode == 1 && comparison == LESS)
                    count = bound - initial;
                else if (update_mode == 1 && comparison == LESS_EQUAL)
                    count = bound - initial + 1.0;
                else if (update_mode == 2 && comparison == GREATER)
                    count = initial - bound;
                else if (update_mode == 2 && comparison == GREATER_EQUAL)
                    count = initial - bound + 1.0;
            }

            if (count >= 0.0 && std::isfinite(count))
            {
                const double result = initial + direction * count;
                if (std::isfinite(result) && std::abs(result) <= max_exact_integer &&
                    !compare_values(Value(result), limit, comparison))
                {
                    target = result;
                    break;
                }
            }

            double current = initial;
            while (compare_values(Value(current), limit, comparison))
                current += direction;
            target = current;
            break;
        }

        case OpCode::GET_LOCAL:
        {
            uint8_t slot = read_byte();

            // Another debugging line, keep it here
            /* std::cout << "[DEBUG] GET_LOCAL slot=" << int(slot)
                      << ", stack.size=" << stack.size();

            if (!frames.empty())
                std::cout << ", frame.stack_start=" << frames.back().stack_start;

            std::cout << std::endl; */

            if (frames.empty())
                throw std::runtime_error("GET_LOCAL called with no active frame");

            size_t idx = frames.back().stack_start + slot;
            if (idx >= stack.size())
            {
                std::cerr << "[ERROR] GET_LOCAL out of bounds, idx=" << idx
                          << ", stack.size=" << stack.size() << std::endl;
                throw std::runtime_error("GET_LOCAL out of bounds");
            }

            stack.push_back(stack[idx]);
            break;
        }

        case OpCode::SET_LOCAL:
        {
            uint8_t slot = read_byte();

            // Keep this here for debug
            /* std::cout << "[DEBUG] SET_LOCAL slot=" << int(slot)
                      << ", stack.size=" << stack.size();

            if (!frames.empty())
                std::cout << ", frame.stack_start=" << frames.back().stack_start;

            std::cout << std::endl; */

            if (frames.empty())
                throw std::runtime_error("SET_LOCAL called with no active frame");

            size_t idx = frames.back().stack_start + slot;
            if (idx >= stack.size())
            {
                std::cerr << "[ERROR] SET_LOCAL out of bounds, idx=" << idx
                          << ", stack.size=" << stack.size() << std::endl;
                throw std::runtime_error("SET_LOCAL out of bounds");
            }

            stack[idx] = stack.back();
            break;
        }

        case OpCode::JUMP:
        {
            uint16_t offset = read_short();
            ip += offset;
            break;
        }

        case OpCode::JUMP_IF_TRUE:
        {
            uint16_t offset = read_short();
            Value condition = pop();
            if (is_truthy(condition))
                ip += offset;
            break;
        }

        case OpCode::JUMP_IF_FALSE:
        {
            uint16_t offset = read_short();
            Value condition = pop();
            if (!is_truthy(condition))
                ip += offset;
            break;
        }

        case OpCode::LOOP:
        {
            uint16_t offset = read_short();
            ip -= offset;
            break;
        }

        case OpCode::CLOSURE:
        {
            uint16_t constant_index = read_short();
            Value constant = chunk->constants.at(constant_index);

            if (!value_is<std::shared_ptr<Function>>(constant))
                throw std::runtime_error("CLOSURE constant is not a function");

            stack.push_back(constant);
            break;
        }

        case OpCode::CALL:
        {
            uint8_t arg_count = read_byte();

            // For debugging calls
            // std::cout << "[DEBUG] CALL with " << int(arg_count) << " args" << std::endl;

            if (stack.size() < arg_count + 1)
                throw std::runtime_error("Not enough values on the stack for CALL");

            Value callee = stack[stack.size() - 1 - arg_count];
            size_t callee_index = stack.size() - 1 - arg_count;

            if (value_is<std::shared_ptr<Class>>(callee))
            {
                auto klass = value_get<std::shared_ptr<Class>>(callee);
                size_t callee_index = stack.size() - 1 - arg_count;
                auto instance = std::make_shared<Instance>(klass);

                stack[callee_index] = instance;

                auto init = klass->find_method("init");

                if (init)
                {
                    if (arg_count != init->arity)
                    {
                        throw std::runtime_error(
                            "Constructor expected " +
                            std::to_string(init->arity) +
                            " arguments but got " +
                            std::to_string(arg_count));
                    }

                    frames.push_back({chunk, ip, callee_index, true, init});

                    chunk = &init->chunk;
                    ip = chunk->code.data();

                    continue;
                }

                stack.erase(stack.begin() + callee_index + 1, stack.begin() + callee_index + 1 + arg_count);
                break;
            }

            else if (value_is<std::shared_ptr<UserBoundMethod>>(callee))
            {
                auto bound = value_get<std::shared_ptr<UserBoundMethod>>(callee);
                auto func = bound->method;
                auto receiver = bound->receiver;

                const std::string caller_class = !frames.empty() && frames.back().function
                    ? frames.back().function->owner_class
                    : std::string{};
                if (!func->is_public && func->owner_class != caller_class)
                    throw std::runtime_error("Cannot call private method \"" + func->name + "\"");

                if (arg_count != func->arity)
                    throw std::runtime_error(
                        "Expected " + std::to_string(func->arity) +
                        " arguments but got " + std::to_string(arg_count));

                size_t callee_index = stack.size() - 1 - arg_count;

                stack[callee_index] = receiver;

                frames.push_back({chunk, ip, callee_index, false, func});

                chunk = &func->chunk;
                ip = chunk->code.data();
                continue;
            }

            if (value_is<std::shared_ptr<BoundMethod>>(callee))
            {
                size_t local_index = stack.size() - 1 - arg_count;
                auto method = value_get<std::shared_ptr<BoundMethod>>(stack[local_index]);
                auto &args = native_args;
                args.clear();
                args.reserve(arg_count);
                for (size_t i = 0; i < arg_count; ++i)
                    args.push_back(std::move(stack[local_index + 1 + i]));
                stack.resize(local_index);

                switch (method->method)
                {
                case NativeMethod::LED_ON:
                    if (!args.empty()) throw std::runtime_error("led_on() takes no arguments");
#ifdef ESP_PLATFORM
                    carblang_led_on();
#else
                    throw std::runtime_error("LED control is only available on ESP32");
#endif
                    push(nullptr);
                    break;
                case NativeMethod::LED_OFF:
                    if (!args.empty()) throw std::runtime_error("led_off() takes no arguments");
#ifdef ESP_PLATFORM
                    carblang_led_off();
#else
                    throw std::runtime_error("LED control is only available on ESP32");
#endif
                    push(nullptr);
                    break;
                case NativeMethod::LED_TOGGLE:
                    if (!args.empty()) throw std::runtime_error("led_toggle() takes no arguments");
#ifdef ESP_PLATFORM
                    carblang_led_toggle();
#else
                    throw std::runtime_error("LED control is only available on ESP32");
#endif
                    push(nullptr);
                    break;
                    NATIVE_GLOBALS_INPUT
                    NATIVE_GLOBALS_RAND
                    NATIVE_GLOBALS_RANDINT
                    NATIVE_GLOBALS_FILL
                    NATIVE_GLOBALS_INIT
                    NATIVE_GLOBALS_ARRAY_INPUT
                    NATIVE_GLOBALS_FORMAT
                    NATIVE_GLOBALS_PRINTF
                    NATIVE_GLOBALS_PRINT
                    NATIVE_GLOBALS_PRINTLN
                    NATIVE_GLOBALS_PARSE_JSON
                    NATIVE_GLOBALS_TO_JSON
                    NATIVE_GLOBALS_PARSE_CSV
                    NATIVE_GLOBALS_TO_CSV
                    NATIVE_GLOBALS_STRINGIFY
                    NATIVE_GLOBALS_INSTANCE_METHODS

                    NATIVE_GLOBALS_POW
                    NATIVE_GLOBALS_SQRT
                    NATIVE_GLOBALS_FACT
                    NATIVE_GLOBALS_FLOOR
                    NATIVE_GLOBALS_CEIL
                    NATIVE_GLOBALS_ABS
                    NATIVE_GLOBALS_LOG
                    NATIVE_GLOBALS_LOG10
                    NATIVE_GLOBALS_LOG2
                    NATIVE_GLOBALS_MATH_MAX
                    NATIVE_GLOBALS_MATH_MIN
                    NATIVE_GLOBALS_MATH_AVERAGE
                    NATIVE_GLOBALS_SIN
                    NATIVE_GLOBALS_COS
                    NATIVE_GLOBALS_TAN
                    NATIVE_GLOBALS_ASIN
                    NATIVE_GLOBALS_ACOS
                    NATIVE_GLOBALS_ATAN
                    NATIVE_GLOBALS_DEGREES
                    NATIVE_GLOBALS_RADIANS
                    NATIVE_GLOBALS_IS_ODD
                    NATIVE_GLOBALS_IS_EVEN
                    NATIVE_GLOBALS_IS_PRIME
                    NATIVE_GLOBALS_GCD
                    NATIVE_GLOBALS_LCM
                    NATIVE_GLOBALS_NEXT_PRIME
                    NATIVE_GLOBALS_SUM_DIGITS
                    NATIVE_GLOBALS_FIBONACCI
                    NATIVE_GLOBALS_PALINDROME
                    NATIVE_GLOBALS_ROUND
                    NATIVE_GLOBALS_PI
                    NATIVE_GLOBALS_PRECISION

                    NATIVE_GLOBALS_RANDOM_COLOR_RGB
                    NATIVE_GLOBALS_RANDOM_COLOR_HEX

                    NATIVE_GLOBALS_MATCH

                    NATIVE_GLOBALS_DATE
                    NATIVE_GLOBALS_SECOND_TO_MINUTE
                    NATIVE_GLOBALS_SECOND_TO_HOUR
                    NATIVE_GLOBALS_SECOND_TO_DAY
                    NATIVE_GLOBALS_MINUTE_TO_SECOND
                    NATIVE_GLOBALS_MINUTE_TO_HOUR
                    NATIVE_GLOBALS_MINUTE_TO_DAY
                    NATIVE_GLOBALS_HOUR_TO_SECOND
                    NATIVE_GLOBALS_HOUR_TO_MINUTE
                    NATIVE_GLOBALS_HOUR_TO_DAY
                    NATIVE_GLOBALS_LEAP_YEAR
                    NATIVE_GLOBALS_TODAY

                    NATIVE_GLOBALS_BASE


                    NATIVE_GLOBALS_PROFILE_START
                    NATIVE_GLOBALS_PROFILE_END
                    NATIVE_GLOBALS_PROFILE_REPORT
                    NATIVE_GLOBALS_PROFILE_RESET

                    NATIVE_ARRAY_PUSH
                    NATIVE_ARRAY_POP
                    NATIVE_ARRAY_LENGTH
                    NATIVE_ARRAY_IS_EMPTY
                    NATIVE_ARRAY_CLEAR
                    NATIVE_ARRAY_CONTAINS
                    NATIVE_ARRAY_INDEX_OF
                    NATIVE_ARRAY_JOIN
                    NATIVE_ARRAY_INSERT
                    NATIVE_ARRAY_REMOVE_AT
                    NATIVE_ARRAY_REVERSE
                    NATIVE_ARRAY_FIRST
                    NATIVE_ARRAY_LAST
                    NATIVE_ARRAY_SLICE
                    NATIVE_ARRAY_COPY
                    NATIVE_ARRAY_CONCAT
                    NATIVE_ARRAY_SWAP
                    NATIVE_ARRAY_COUNT
                    NATIVE_ARRAY_EQUALS
                    NATIVE_ARRAY_LAST_INDEX_OF
                    NATIVE_ARRAY_SUM
                    NATIVE_ARRAY_MIN
                    NATIVE_ARRAY_MAX
                    NATIVE_ARRAY_AVERAGE
                    NATIVE_ARRAY_TRIM

                    NATIVE_PRIMITIVE_TYPE
                    NATIVE_PRIMITIVE_TO_STRING
                    NATIVE_PRIMITIVE_TO_INT
                    NATIVE_PRIMITIVE_BOOL_TO_INT
                    NATIVE_PRIMITIVE_UPPER
                    NATIVE_PRIMITIVE_LOWER
                    NATIVE_PRIMITIVE_CAPITALIZE
                    NATIVE_PRIMITIVE_SWAPCASE
                    NATIVE_PRIMITIVE_FIND
                    NATIVE_PRIMITIVE_FIND_LAST
                    NATIVE_PRIMITIVE_STR_FIRST
                    NATIVE_PRIMITIVE_STR_LAST
                    NATIVE_PRIMITIVE_STARTS_WITH
                    NATIVE_PRIMITIVE_ENDS_WITH
                    NATIVE_PRIMITIVE_IS_NUMBER
                    NATIVE_PRIMITIVE_STR_TRIM
                    NATIVE_PRIMITIVE_TO_ARRAY
                    NATIVE_PRIMITIVE_REPLACE
                    NATIVE_PRIMITIVE_IS_SPACE
                    NATIVE_PRIMITIVE_IS_ALL_SPACES
                    NATIVE_PRIMITIVE_STR_IS_EMPTY
                    NATIVE_PRIMITIVE_STR_LENGTH
                    NATIVE_PRIMITIVE_STR_SLICE
                    NATIVE_PRIMITIVE_STR_COUNT
                    NATIVE_PRIMITIVE_TO_NUMBER

                    NATIVE_DICT_CONTAINS
                    NATIVE_DICT_REMOVE
                    NATIVE_DICT_LENGTH
                    NATIVE_DICT_KEYS
                    NATIVE_DICT_VALUES
                    NATIVE_DICT_POP
                    NATIVE_DICT_COPY
                    NATIVE_DICT_CLEAR

                default:
                    throw std::runtime_error("Unrecognized method");
                }

                args.clear();
                continue;
            }

            if (!value_is<std::shared_ptr<Function>>(callee))
                throw std::runtime_error("Can only call functions");

            auto function = value_get<std::shared_ptr<Function>>(callee);

            if (arg_count != function->arity)
            {
                throw std::runtime_error(
                    "Expected " + std::to_string(function->arity) +
                    " arguments but got " + std::to_string(arg_count));
            }

            frames.push_back({chunk, ip, callee_index, false, function});

            stack.erase(stack.begin() + callee_index);
            chunk = &function->chunk;
            ip = chunk->code.data();
            break;
        }

        case OpCode::RETURN:
        {
            if (frames.empty())
                return;

            CallFrame frame = frames.back();
            frames.pop_back();

            Value return_value;

            if (frame.is_constructor)
            {
                // print_stack();
                if (stack.size() > frame.stack_start + 1)
                {
                    Value v = pop();
                    if (!value_is<IgnoreReturnValue>(v))
                    {
                        if (value_is<std::nullptr_t>(v) || value_is<std::shared_ptr<Instance>>(v))
                        {
                            return_value = v;
                            stack.resize(frame.stack_start);
                            stack.push_back(return_value);
                            chunk = frame.chunk;
                            ip = frame.ip;
                            break;
                        }
                        else
                        {
                            throw std::runtime_error("The class method init can only return null or the instance itself");
                        }
                    }
                }

                return_value = stack[frame.stack_start];
                stack.resize(frame.stack_start);
            }

            else
            {
                return_value = pop();
                stack.resize(frame.stack_start);
            }

            stack.push_back(return_value);
            chunk = frame.chunk;
            ip = frame.ip;
            break;
        }

        case OpCode::ARRAY:
        {
            uint8_t count = read_byte();

            auto array = std::make_shared<Array>();
            array->elements.resize(count);
            for (int i = count - 1; i >= 0; --i)
                array->elements[i] = pop();

            stack.push_back(array);
            break;
        }

        case OpCode::GET_INDEX:
        {
            Value index_val = pop();
            Value container_val = pop();

            if (value_is<std::shared_ptr<Array>>(container_val))
            {
                auto array = value_get<std::shared_ptr<Array>>(container_val);
                size_t index = checked_sequence_index(index_val, array->elements.size(), "Array");
                push(array->elements[index]);
            }
            else if (value_is<std::string>(container_val))
            {
                const auto &str = value_get<std::string>(container_val);
                size_t index = checked_sequence_index(index_val, str.size(), "String");
                push(std::string(1, str[index]));
            }
            else if (value_is<std::shared_ptr<Dict>>(container_val))
            {
                auto dict = value_get<std::shared_ptr<Dict>>(container_val);
                if (!value_is<std::string>(index_val))
                    throw std::runtime_error("Dictionary key must be string");
                push(dict->get(value_get<std::string>(index_val)));
            }
            else
                throw std::runtime_error("Cannot index this type");

            break;
        }

        case OpCode::SET_INDEX:
        {
            Value value = pop();
            Value index_val = pop();
            Value container_val = pop();

            if (value_is<std::shared_ptr<Array>>(container_val))
            {
                auto array = value_get<std::shared_ptr<Array>>(container_val);
                size_t index = checked_sequence_index(index_val, array->elements.size(), "Array");
                array->elements[index] = value;
                push(value);
            }
            else if (value_is<std::shared_ptr<Dict>>(container_val))
            {
                auto dict = value_get<std::shared_ptr<Dict>>(container_val);
                if (!value_is<std::string>(index_val))
                    throw std::runtime_error("Dictionary key must be string");
                dict->set(value_get<std::string>(index_val), value);
                push(value);
            }
            else
                throw std::runtime_error("Cannot assign index to this type");

            break;
        }

        case OpCode::GET_PROPERTY:
        {
            std::string name = value_get<std::string>(read_constant());
            Value object = pop();

            // For properties stuff
            // std::cout << "[DEBUG] GET_PROPERTY name=" << name
            //    << ", object=" << stringify(object) << std::endl;

            if (name == "type")
            {
                push(make_native_method(object, NativeMethod::TYPE));
                break;
            }

            if (value_is<bool>(object))
            {
                if (name == "to_int")
                {
                    push(make_native_method(object, NativeMethod::BOOL_TO_INT));
                    break;
                }
                if (name == "to_string")
                {
                    push(make_native_method(object, NativeMethod::TO_STRING));
                    break;
                }
                throw std::runtime_error("Undefined property \"" + name + "\"");
                break;
            }

            if (value_is<double>(object))
            {
                if (name == "to_string")
                {
                    push(make_native_method(object, NativeMethod::TO_STRING));
                    break;
                }
                if (name == "to_int")
                {
                    push(make_native_method(object, NativeMethod::TO_INT));
                    break;
                }
                throw std::runtime_error("Undefined property \"" + name + "\"");
                break;
            }

            if (value_is<std::string>(object))
            {
                if (name == "to_number")
                {
                    push(make_native_method(object, NativeMethod::TO_NUMBER));
                    break;
                }
                if (name == "upper")
                {
                    push(make_native_method(object, NativeMethod::UPPER));
                    break;
                }
                if (name == "lower")
                {
                    push(make_native_method(object, NativeMethod::LOWER));
                    break;
                }
                if (name == "capitalize")
                {
                    push(make_native_method(object, NativeMethod::CAPITALIZE));
                    break;
                }
                if (name == "swap")
                {
                    push(make_native_method(object, NativeMethod::SWAPCASE));
                    break;
                }
                if (name == "find")
                {
                    push(make_native_method(object, NativeMethod::FIND));
                    break;
                }
                if (name == "find_last")
                {
                    push(make_native_method(object, NativeMethod::FIND_LAST));
                    break;
                }
                if (name == "first")
                {
                    push(make_native_method(object, NativeMethod::STR_FIRST));
                    break;
                }
                if (name == "last")
                {
                    push(make_native_method(object, NativeMethod::STR_LAST));
                    break;
                }
                if (name == "starts_with")
                {
                    push(make_native_method(object, NativeMethod::STARTS_WITH));
                    break;
                }
                if (name == "ends_with")
                {
                    push(make_native_method(object, NativeMethod::ENDS_WITH));
                    break;
                }
                if (name == "is_number")
                {
                    push(make_native_method(object, NativeMethod::IS_NUMBER));
                    break;
                }
                if (name == "is_space")
                {
                    push(make_native_method(object, NativeMethod::IS_SPACE));
                    break;
                }
                if (name == "is_all_spaces")
                {
                    push(make_native_method(object, NativeMethod::IS_ALL_SPACES));
                    break;
                }
                if (name == "replace")
                {
                    push(make_native_method(object, NativeMethod::REPLACE));
                    break;
                }
                if (name == "trim")
                {
                    push(make_native_method(object, NativeMethod::STR_TRIM));
                    break;
                }
                if (name == "to_array")
                {
                    push(make_native_method(object, NativeMethod::TO_ARRAY));
                    break;
                }
                if (name == "is_empty")
                {
                    push(make_native_method(object, NativeMethod::STR_IS_EMPTY));
                    break;
                }
                if (name == "length")
                {
                    push(make_native_method(object, NativeMethod::STR_LENGTH));
                    break;
                }
                if (name == "count")
                {
                    push(make_native_method(object, NativeMethod::STR_COUNT));
                    break;
                }
                if (name == "slice")
                {
                    push(make_native_method(object, NativeMethod::STR_SLICE));
                    break;
                }
                throw std::runtime_error("Undefined property \"" + name + "\"");
                break;
            }

            if (value_is<std::shared_ptr<Array>>(object))
            {
                auto array = value_get<std::shared_ptr<Array>>(object);

                if (name == "length")
                {
                    push(make_native_method(array, NativeMethod::LENGTH));
                    break;
                }
                if (name == "push")
                {
                    push(make_native_method(array, NativeMethod::PUSH));
                    break;
                }
                if (name == "pop")
                {
                    push(make_native_method(array, NativeMethod::POP));
                    break;
                }
                if (name == "is_empty")
                {
                    push(make_native_method(array, NativeMethod::IS_EMPTY));
                    break;
                }
                if (name == "clear")
                {
                    push(make_native_method(array, NativeMethod::CLEAR));
                    break;
                }
                if (name == "contains")
                {
                    push(make_native_method(array, NativeMethod::CONTAINS));
                    break;
                }
                if (name == "first_index")
                {
                    push(make_native_method(array, NativeMethod::INDEX_OF));
                    break;
                }
                if (name == "insert")
                {
                    push(make_native_method(array, NativeMethod::INSERT));
                    break;
                }
                if (name == "remove")
                {
                    push(make_native_method(array, NativeMethod::REMOVE_AT));
                    break;
                }
                if (name == "join")
                {
                    push(make_native_method(array, NativeMethod::JOIN));
                    break;
                }
                if (name == "last_index")
                {
                    push(make_native_method(array, NativeMethod::LAST_INDEX_OF));
                    break;
                }
                if (name == "equals")
                {
                    push(make_native_method(array, NativeMethod::EQUALS));
                    break;
                }
                if (name == "count")
                {
                    push(make_native_method(array, NativeMethod::COUNT));
                    break;
                }
                if (name == "swap")
                {
                    push(make_native_method(array, NativeMethod::SWAP));
                    break;
                }
                if (name == "concat")
                {
                    push(make_native_method(array, NativeMethod::CONCAT));
                    break;
                }
                if (name == "copy")
                {
                    push(make_native_method(array, NativeMethod::COPY));
                    break;
                }
                if (name == "slice")
                {
                    push(make_native_method(array, NativeMethod::SLICE));
                    break;
                }
                if (name == "last")
                {
                    push(make_native_method(array, NativeMethod::LAST));
                    break;
                }
                if (name == "first")
                {
                    push(make_native_method(array, NativeMethod::FIRST));
                    break;
                }
                if (name == "reverse")
                {
                    push(make_native_method(array, NativeMethod::REVERSE));
                    break;
                }
                if (name == "trim")
                {
                    push(make_native_method(array, NativeMethod::TRIM));
                    break;
                }
                if (name == "sum")
                {
                    push(make_native_method(array, NativeMethod::SUM));
                    break;
                }
                if (name == "min")
                {
                    push(make_native_method(array, NativeMethod::MIN));
                    break;
                }
                if (name == "max")
                {
                    push(make_native_method(array, NativeMethod::MAX));
                    break;
                }
                if (name == "average")
                {
                    push(make_native_method(array, NativeMethod::AVERAGE));
                    break;
                }
                throw std::runtime_error("Undefined property \"" + name + "\"");
                break;
            }


            if (value_is<std::shared_ptr<Dict>>(object))
            {
                auto dict = value_get<std::shared_ptr<Dict>>(object);

                if (name == "contains")
                {
                    push(make_native_method(dict, NativeMethod::DICT_CONTAINS));
                    break;
                }
                if (name == "remove")
                {
                    push(make_native_method(dict, NativeMethod::DICT_REMOVE));
                    break;
                }
                if (name == "length")
                {
                    push(make_native_method(dict, NativeMethod::DICT_LENGTH));
                    break;
                }
                if (name == "keys")
                {
                    push(make_native_method(dict, NativeMethod::DICT_KEYS));
                    break;
                }
                if (name == "values")
                {
                    push(make_native_method(dict, NativeMethod::DICT_VALUES));
                    break;
                }
                if (name == "pop")
                {
                    push(make_native_method(dict, NativeMethod::DICT_POP));
                    break;
                }
                if (name == "copy")
                {
                    push(make_native_method(dict, NativeMethod::DICT_COPY));
                    break;
                }
                if (name == "clear")
                {
                    push(make_native_method(dict, NativeMethod::DICT_CLEAR));
                    break;
                }
                throw std::runtime_error("Undefined property \"" + name + "\"");
                break;
            }

            if (value_is<std::shared_ptr<Instance>>(object))
            {
                auto instance = value_get<std::shared_ptr<Instance>>(object);
                const std::string caller_class = !frames.empty() && frames.back().function
                    ? frames.back().function->owner_class
                    : std::string{};
                const PropertyInfo *property = instance->klass->find_property(name);
                if (property && !property->is_public && property->owner_class != caller_class)
                    throw std::runtime_error("Cannot access private property \"" + name + "\"");

                auto it = instance->fields.find(name);
                if (it != instance->fields.end())
                {
                    push(it->second);
                    break;
                }

                std::shared_ptr<Function> method;
                auto cached = method_cache.find(instruction);
                if (cached != method_cache.end() && cached->second.klass == instance->klass)
                    method = cached->second.method;
                else
                {
                    method = instance->klass->find_method(name);
                    if (method)
                        method_cache.insert_or_assign(
                            instruction, MethodCacheEntry{instance->klass, method});
                }
                if (method)
                {
                    if (!method->is_public && method->owner_class != caller_class)
                        throw std::runtime_error("Cannot access private method \"" + name + "\"");
                    push(std::make_shared<UserBoundMethod>(instance, method));
                    break;
                }

                throw std::runtime_error("Undefined property \"" + name + "\"");
            }

            else
            {
                throw std::runtime_error("Cannot get property \"" + name + "\" of this value");
            }
        }

        case OpCode::DICT:
        {
            stack.push_back(std::make_shared<Dict>());
            break;
        }

        case OpCode::GET_ITEM:
        {
            Value key = pop();
            Value dict_val = pop();

            if (auto dict = value_get_if<std::shared_ptr<Dict>>(&dict_val))
            {
                if (!value_is<std::string>(key))
                    throw std::runtime_error("Dictionary key must be a string");

                stack.push_back((*dict)->get(value_get<std::string>(key)));
            }
            else
                throw std::runtime_error("Not a dictionary");

            break;
        }

        case OpCode::SET_ITEM:
        {
            Value val = pop();
            Value key = pop();
            Value dict_val = pop();

            if (auto dict = value_get_if<std::shared_ptr<Dict>>(&dict_val))
            {
                if (!value_is<std::string>(key))
                    throw std::runtime_error("Dictionary key must be a string");

                (*dict)->set(value_get<std::string>(key), std::move(val));
                stack.push_back(dict_val);
            }
            else
                throw std::runtime_error("Not a dictionary");

            break;
        }

        case OpCode::SET_PROPERTY:
        {
            Value name_val = read_constant();
            if (!value_is<std::string>(name_val))
                throw std::runtime_error("Property name must be a string");
            std::string name = value_get<std::string>(name_val);

            Value value = pop();
            Value object = pop();
            if (!value_is<std::shared_ptr<Instance>>(object))
                throw std::runtime_error("Only instances have fields");
            auto instance = value_get<std::shared_ptr<Instance>>(std::move(object));

            const std::string caller_class = !frames.empty() && frames.back().function
                ? frames.back().function->owner_class
                : std::string{};
            const PropertyInfo *property = instance->klass->find_property(name);
            if (property && !property->is_public && property->owner_class != caller_class)
                throw std::runtime_error("Cannot access private property \"" + name + "\"");

            if (!property && instance->klass->is_user_defined)
            {
                const bool in_constructor = !frames.empty() && frames.back().function &&
                    frames.back().function->name == "init" &&
                    frames.back().function->owner_class == instance->klass->name;
                if (!in_constructor)
                    throw std::runtime_error("Property \"" + name +
                                             "\" must be declared in init()");
            }

            if (property && property->type_name && !runtime_type_matches(value, *property->type_name))
                throw std::runtime_error("Property \"" + name + "\" expects type \"" +
                                         *property->type_name + "\"");

            instance->fields[name] = value;
            stack.push_back(std::move(value));
            break;
        }

        case OpCode::CLASS:
        {
            uint16_t name_index = read_short();
            Value name_val = chunk->constants.at(name_index);

            if (!value_is<std::string>(name_val))
                throw std::runtime_error("Class name must be a string");

            std::string name = value_get<std::string>(name_val);

            auto klass = std::make_shared<Class>(
                name,
                std::unordered_map<std::string, std::shared_ptr<Function>>{});
            klass->is_user_defined = true;

            push(klass);
            break;
        }

        case OpCode::INHERIT:
        {
            auto sub_class_val = pop();
            auto super_class_val = pop();

            if (!value_is<std::shared_ptr<Class>>(super_class_val) ||
                !value_is<std::shared_ptr<Class>>(sub_class_val))
                throw std::runtime_error("INHERIT expects two classes on stack");

            auto super_class = value_get<std::shared_ptr<Class>>(super_class_val);
            auto sub_class = value_get<std::shared_ptr<Class>>(sub_class_val);
            if (super_class == sub_class)
                throw std::runtime_error("A class cannot inherit from itself");
            for (auto parent = super_class; parent; parent = parent->super_class)
                if (parent == sub_class)
                    throw std::runtime_error("Circular class inheritance");
            sub_class->super_class = super_class;
            for (auto &[method_name, method] : sub_class->methods)
                method->super_class = super_class;

            stack.push_back(sub_class);
            break;
        }

        case OpCode::GET_SUPER:
        {
            Value method_name_value = read_constant();
            if (!value_is<std::string>(method_name_value))
                throw std::runtime_error("Superclass method name must be a string");
            const auto &method_name = value_get<std::string>(method_name_value);
            if (frames.empty() || !frames.back().function || !frames.back().function->super_class)
                throw std::runtime_error("No superclass for this method");
            auto instance = current_instance();
            auto method = frames.back().function->super_class->find_method(method_name);
            if (!method)
                throw std::runtime_error("Undefined superclass method \"" + method_name + "\"");
            push(std::make_shared<UserBoundMethod>(instance, method));
            break;
        }

        case OpCode::METHOD:
        {
            uint16_t name_index = read_short();
            Value name_val = chunk->constants.at(name_index);

            if (!value_is<std::string>(name_val))
                throw std::runtime_error("Method name must be a string");

            std::string name = value_get<std::string>(name_val);

            auto function = pop_function();
            auto klass = pop_class();

            if (klass->methods.contains(name))
                throw std::runtime_error(
                    "Class \"" + klass->name + "\" already defines method \"" + name + "\"");

            klass->methods[name] = function;
            if (name == "init")
                klass->properties = function->declared_properties;
            push(klass);
            break;
        }

        case OpCode::NULL:
            stack.push_back(nullptr);
            break;

        case OpCode::TRUE:
            stack.push_back(true);
            break;

        case OpCode::FALSE:
            stack.push_back(false);
            break;

        default:
            throw std::runtime_error("Unknown opcode");
        }
    }
}

Value VM::pop()
{
    if (stack.empty())
    {
        throw std::runtime_error("pop() called on empty stack");
    }
    Value v = std::move(stack.back());
    stack.pop_back();
    return v;
}

Value VM::add_one(const Value &value)
{
    if (const double *number = value_get_if<double>(&value))
        return *number + 1.0;

    if (value_is<std::string>(value))
        return stringify(value) + "1";

    if (const auto *array = value_get_if<std::shared_ptr<Array>>(&value))
    {
        auto result = std::make_shared<Array>();
        result->elements.reserve((*array)->elements.size());
        for (const Value &element : (*array)->elements)
        {
            const double *number = value_get_if<double>(&element);
            if (!number)
                throw std::runtime_error("Array must contain only numbers");
            result->elements.push_back(*number + 1.0);
        }
        return result;
    }

    throw std::runtime_error("Operands must be numbers: " + stringify(value) + " 1");
}

Value VM::pop_constant()
{
    return pop();
}

std::shared_ptr<Function> VM::pop_function()
{
    Value v = pop();
    if (!value_is<std::shared_ptr<Function>>(v))
    {
        throw std::runtime_error("Expected a function on the stack");
    }
    return value_get<std::shared_ptr<Function>>(v);
}

std::shared_ptr<Class> VM::pop_class()
{
    Value v = pop();
    if (!value_is<std::shared_ptr<Class>>(v))
    {
        throw std::runtime_error("Expected a class on the stack");
    }
    return value_get<std::shared_ptr<Class>>(v);
}

std::shared_ptr<Instance> VM::pop_instance()
{
    Value val = pop();
    if (!value_is<std::shared_ptr<Instance>>(val))
        throw std::runtime_error("Expected Instance");
    return value_get<std::shared_ptr<Instance>>(val);
}

void VM::push(Value value)
{
    stack.push_back(std::move(value));
}

uint16_t VM::read_short()
{
    const uint8_t *end = chunk->code.data() + chunk->code.size();
    if (ip >= end || end - ip < 2)
        throw std::runtime_error("read_short() past end of bytecode");

    uint16_t high = *ip++;
    uint16_t low = *ip++;
    return (high << 8) | low;
}

uint8_t VM::read_byte()
{
    if (ip >= chunk->code.data() + chunk->code.size())
        throw std::runtime_error("read_byte() past end of bytecode");

    return *ip++;
}

Value VM::read_constant()
{
    return chunk->constants.at(read_short());
}

std::shared_ptr<Instance> VM::current_instance()
{
    if (frames.empty())
        throw std::runtime_error("No active frame for current_instance()");

    size_t idx = frames.back().stack_start;
    Value val = stack.at(idx);

    if (!value_is<std::shared_ptr<Instance>>(val))
        throw std::runtime_error("Expected instance at stack_start");

    return value_get<std::shared_ptr<Instance>>(val);
}

bool VM::is_truthy(const Value &value)
{
    if (value_is<std::nullptr_t>(value))
        return false;
    if (auto b = value_get_if<bool>(&value))
        return *b;
    return true;
}

bool VM::value_shallow_equal(const Value &x, const Value &y)
{
    if (x.kind() != y.kind())
        return false;

    if (value_is<std::shared_ptr<Array>>(x))
        return value_get<std::shared_ptr<Array>>(x) ==
               value_get<std::shared_ptr<Array>>(y);

    return x == y;
}

void VM::print_value(const Value &value)
{
    write_output(this->stringify(value));
}

void VM::write_output(const std::string &text)
{
#ifdef ESP_PLATFORM
    if (!text.empty())
        carblang_uart_write(text.data(), text.size());
#else
    std::cout << text;
#endif
}

std::string VM::stringify(const Value &value)
{
    std::ostringstream out;
    if (value_is<std::nullptr_t>(value))
    {
        out << "null";
    }

    else if (value_is<bool>(value))
    {
        out << (value_get<bool>(value) ? "true" : "false");
    }

    else if (value_is<double>(value))
    {
        double d = value_get<double>(value);

        if (std::isinf(d))
        {
            out << (d > 0 ? "Infinity" : "-Infinity");
        }
        else
        {
            std::ostringstream oss;
            oss << std::setprecision(std::numeric_limits<double>::digits10 + 1);

            if ((d != 0.0 && (std::abs(d) < 1e-4 || std::abs(d) >= 1e7)))
            {
                oss << std::scientific << d;
            }
            else
            {
                oss << std::defaultfloat << d;
            }

            out << oss.str();
        }
    }

    else if (value_is<std::string>(value))
    {
        out << value_get<std::string>(value);
    }

    else if (value_is<std::shared_ptr<Array>>(value))
    {
        auto array = value_get<std::shared_ptr<Array>>(value);
        out << "[";

        for (size_t i = 0; i < array->elements.size(); ++i)
        {
            const auto &elem = array->elements[i];

            if (value_is<std::string>(elem))
                out << "\"" << value_get<std::string>(elem) << "\"";
            else
                out << stringify(elem);

            if (i + 1 < array->elements.size())
                out << ", ";
        }

        out << "]";
    }

    else if (value_is<std::shared_ptr<Function>>(value))
    {
        out << "<function>";
    }

    else if (value_is<std::shared_ptr<Class>>(value))
    {
        auto klass = value_get<std::shared_ptr<Class>>(value);
        out << "<class " << klass->name << ">";
    }

    else if (value_is<std::shared_ptr<Instance>>(value))
    {
        auto instance = value_get<std::shared_ptr<Instance>>(value);
        out << "<" << instance->klass->name << " instance>";
    }

    else if (value_is<std::shared_ptr<UserBoundMethod>>(value))
    {
        out << "<bound method>";
    }

    else if (value_is<std::shared_ptr<BoundMethod>>(value))
    {
        out << "<native method>";
    }

    else if (value_is<IgnoreReturnValue>(value))
    {
        out << "<ignore>";
    }

    else if (value_is<std::shared_ptr<Dict>>(value))
    {
        auto dict = value_get<std::shared_ptr<Dict>>(value);
        out << "{";
        size_t count = 0;

        for (const auto &[key, val] : dict->entries)
        {
            out << "\"" << key << "\": ";

            if (value_is<std::string>(val))
                out << "\"" << value_get<std::string>(val) << "\"";
            else
                out << stringify(val);

            if (++count < dict->entries.size())
                out << ", ";
        }

        out << "}";
    }

    else
    {
        out << "<unknown>";
    }

    return out.str();
}

std::string VM::json_escape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());

    for (unsigned char c : s)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;

        default:
        {
            if (c < 0x20)
            {
                std::ostringstream ss;
                ss << "\\u"
                   << std::hex
                   << std::uppercase
                   << std::setw(4)
                   << std::setfill('0')
                   << (int)c;
                out += ss.str();
            }
            else
            {
                out += c;
            }
        }
        }
    }

    return out;
}

std::string VM::json_number(double value)
{
    std::ostringstream out;
    out << std::setprecision(15) << value;

    std::string s = out.str();

    if (s.find('.') != std::string::npos)
    {
        while (!s.empty() && s.back() == '0')
            s.pop_back();

        if (!s.empty() && s.back() == '.')
            s.pop_back();
    }

    return s;
}

std::string VM::json_stringify(const Value &value)
{
    if (value_is<std::nullptr_t>(value))
        return "null";

    if (value_is<bool>(value))
        return value_get<bool>(value) ? "true" : "false";

    if (value_is<double>(value))
        return json_number(value_get<double>(value));

    if (value_is<std::string>(value))
        return "\"" + json_escape(value_get<std::string>(value)) + "\"";

    if (value_is<std::shared_ptr<Array>>(value))
    {
        auto arr = value_get<std::shared_ptr<Array>>(value);

        std::string out = "[";
        for (size_t i = 0; i < arr->elements.size(); ++i)
        {
            out += json_stringify(arr->elements[i]);
            if (i + 1 < arr->elements.size())
                out += ",";
        }
        out += "]";
        return out;
    }

    if (value_is<std::shared_ptr<Dict>>(value))
    {
        auto dict = value_get<std::shared_ptr<Dict>>(value);

        std::string out = "{";
        size_t count = 0;

        for (const auto &[key, val] : dict->entries)
        {
            out += "\"";
            out += json_escape(key);
            out += "\":";
            out += json_stringify(val);

            if (++count < dict->entries.size())
                out += ",";
        }

        out += "}";
        return out;
    }

    throw std::runtime_error("Value cannot be serialized to JSON");
}

std::string VM::csv_escape(const std::string &s)
{
    bool needs_quotes = false;

    for (char c : s)
    {
        if (c == ',' || c == '"' || c == '\n' || c == '\r')
        {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes)
        return s;

    std::string out = "\"";

    for (char c : s)
    {
        if (c == '"')
            out += "\"\"";
        else
            out += c;
    }

    out += "\"";
    return out;
}

std::string VM::csv_value(const Value &value)
{
    if (value_is<std::nullptr_t>(value))
        return "";

    if (value_is<bool>(value))
        return value_get<bool>(value) ? "true" : "false";

    if (value_is<double>(value))
        return json_number(value_get<double>(value));

    if (value_is<std::string>(value))
        return csv_escape(value_get<std::string>(value));

    throw std::runtime_error("Unsupported CSV value type (no nested arrays/objects)");
}

std::string VM::csv_stringify(const Value &value)
{
    if (!value_is<std::shared_ptr<Array>>(value))
        throw std::runtime_error("CSV must be an array of rows");

    auto rows = value_get<std::shared_ptr<Array>>(value);

    std::string out;

    for (size_t i = 0; i < rows->elements.size(); ++i)
    {
        const Value &row_val = rows->elements[i];

        if (!value_is<std::shared_ptr<Array>>(row_val))
            throw std::runtime_error("Each CSV row must be an array");

        auto row = value_get<std::shared_ptr<Array>>(row_val);

        for (size_t j = 0; j < row->elements.size(); ++j)
        {
            out += csv_value(row->elements[j]);

            if (j + 1 < row->elements.size())
                out += ",";
        }

        if (i + 1 < rows->elements.size())
            out += "\n";
    }

    return out;
}

void VM::print_stack()
{
    std::cout << "=== STACK (" << stack.size() << ") ===\n";

    for (size_t i = 0; i < stack.size(); ++i)
    {
        std::cout << "[" << i << "] ";

        std::cout << stringify(stack[i]) << std::endl;
    }

    std::cout << "====================\n";
}
