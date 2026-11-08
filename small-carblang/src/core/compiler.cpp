#include "core/core.hpp"

namespace
{
std::string type_base(const std::string &type)
{
    const size_t generic_start = type.find('[');
    return generic_start == std::string::npos ? type : type.substr(0, generic_start);
}

std::vector<std::string> type_arguments(const std::string &type)
{
    const size_t open = type.find('[');
    if (open == std::string::npos || type.back() != ']')
        return {};

    std::vector<std::string> arguments;
    size_t argument_start = open + 1;
    int depth = 0;
    for (size_t i = argument_start; i < type.size() - 1; ++i)
    {
        if (type[i] == '[')
            ++depth;
        else if (type[i] == ']')
            --depth;
        else if (type[i] == ',' && depth == 0)
        {
            arguments.push_back(type.substr(argument_start, i - argument_start));
            argument_start = i + 1;
        }
    }
    arguments.push_back(type.substr(argument_start, type.size() - 1 - argument_start));
    return arguments;
}

bool type_matches(const std::string &expected, const std::string &actual)
{
    if (expected == actual || expected == "any" || actual == "any")
        return true;

    const std::string expected_base = type_base(expected);
    const std::string actual_base = type_base(actual);
    if (expected_base != actual_base)
        return false;

    const auto expected_arguments = type_arguments(expected);
    const auto actual_arguments = type_arguments(actual);
    if (expected_arguments.empty())
        return true;
    if (actual_arguments.empty())
        return false;
    if (expected_arguments.size() != actual_arguments.size())
        return false;

    for (size_t i = 0; i < expected_arguments.size(); ++i)
        if (!type_matches(expected_arguments[i], actual_arguments[i]))
            return false;
    return true;
}

bool type_matches_expression(const std::string &expected,
                             const std::string &actual,
                             const std::shared_ptr<Expression> &expression)
{
    if (type_matches(expected, actual))
        return true;

    if (actual != type_base(expected))
        return false;

    if (type_base(expected) == "array")
    {
        auto array = std::dynamic_pointer_cast<ArrayExpr>(expression);
        return array && array->elements.empty();
    }

    if (type_base(expected) == "dict")
    {
        auto dict = std::dynamic_pointer_cast<DictExpr>(expression);
        return dict && dict->entries.empty();
    }

    return false;
}

}

Chunk Compiler::compile(const std::vector<std::shared_ptr<Stmt>> &statements, std::unordered_set<std::string> known_class_types)
{
    chunk = Chunk{};
    locals.clear();
    current_return_type.reset();
    current_method_name.reset();
    current_super_class_name.reset();
    current_class_name.reset();
    scope_depth = 0;
    in_function = false;
    in_method = false;
    current_method_is_public = true;
    in_subclass_method = false;
    discard_assignment_result = false;
    assignment_result_was_discarded = false;
    this->known_class_types.insert(known_class_types.begin(), known_class_types.end());
    for (auto &stmt : statements)
    {
        stmt->accept(*this);
    }

    emit(OpCode::RETURN);
    return chunk;
}

void Compiler::begin_scope()
{
    scope_depth++;
}

void Compiler::end_scope()
{
    scope_depth--;

    while (!locals.empty() && locals.back().depth > scope_depth)
    {
        emit(OpCode::POP);
        locals.pop_back();
    }
}

int Compiler::resolve_local(const std::string &name)
{
    for (int i = locals.size() - 1; i >= 0; i--)
    {
        if (locals[i].name == name)
        {

            return i;
        }
    }
    return -1;
}

void Compiler::emit(OpCode op)
{
    chunk.write(static_cast<uint8_t>(op));
}

void Compiler::emit_byte(size_t byte)
{
    if (byte > UINT8_MAX)
        throw std::runtime_error("Bytecode operand exceeds 255");

    chunk.write(static_cast<uint8_t>(byte));
}

void Compiler::emit_index(uint16_t index)
{
    emit_byte(static_cast<uint8_t>(index >> 8));
    emit_byte(static_cast<uint8_t>(index & 0xff));
}

void Compiler::emit_constant(Value value)
{
    const uint16_t index = chunk.add_constant(std::move(value));
    if (index <= UINT8_MAX)
    {
        emit(OpCode::CONSTANT_SHORT);
        emit_byte(index);
    }
    else
    {
        emit(OpCode::CONSTANT);
        emit_index(index);
    }
}

int Compiler::emit_jump(OpCode op)
{
    emit(op);
    emit_byte(0xff);
    emit_byte(0xff);
    int offset_index = chunk.code.size() - 2;
    return offset_index;
}

std::optional<std::string> Compiler::infer_expression_type(std::shared_ptr<Expression> expr)
{
    if (auto lit = std::dynamic_pointer_cast<Literal>(expr))
    {
        const auto &v = lit->value;

        if (value_is<double>(v))
            return "number";

        if (value_is<std::string>(v))
            return "string";

        if (value_is<bool>(v))
            return "bool";

        if (value_is<std::nullptr_t>(v))
            return "null";

        if (value_is<std::shared_ptr<Function>>(v))
            return "function";

        if (value_is<std::shared_ptr<Class>>(v))
            return "class";

        if (const auto *instance = value_get_if<std::shared_ptr<Instance>>(&v))
            return *instance && (*instance)->klass ? (*instance)->klass->name : "any";

        if (value_is<std::shared_ptr<UserBoundMethod>>(v))
            return "bound_method";

        if (value_is<std::shared_ptr<BoundMethod>>(v))
            return "native_method";

        if (value_is<std::shared_ptr<Dict>>(v))
            return "dict";

        if (value_is<std::shared_ptr<Array>>(v))
            return "array";
    }

    if (auto array = std::dynamic_pointer_cast<ArrayExpr>(expr))
    {
        if (array->elements.empty())
            return "array";

        auto element_type = infer_expression_type(array->elements.front());
        for (size_t i = 1; i < array->elements.size(); ++i)
        {
            auto current_type = infer_expression_type(array->elements[i]);
            if (!element_type || !current_type || !type_matches(*element_type, *current_type) ||
                !type_matches(*current_type, *element_type))
                return "array[mixed]";
        }
        return "array[" + element_type.value_or("any") + "]";
    }

    if (auto dict = std::dynamic_pointer_cast<DictExpr>(expr))
    {
        if (dict->entries.empty())
            return "dict";

        auto key_type = infer_expression_type(dict->entries.front().first);
        auto value_type = infer_expression_type(dict->entries.front().second);
        for (size_t i = 1; i < dict->entries.size(); ++i)
        {
            auto current_key_type = infer_expression_type(dict->entries[i].first);
            auto current_value_type = infer_expression_type(dict->entries[i].second);
            if (!key_type || !current_key_type ||
                !type_matches(*key_type, *current_key_type) ||
                !type_matches(*current_key_type, *key_type))
                key_type = "mixed";
            if (!value_type || !current_value_type ||
                !type_matches(*value_type, *current_value_type) ||
                !type_matches(*current_value_type, *value_type))
                value_type = "mixed";
        }
        return "dict[" + key_type.value_or("any") + "," + value_type.value_or("any") + "]";
    }

    if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr))
    {
        auto container_type = infer_expression_type(index->array);
        if (container_type)
        {
            auto arguments = type_arguments(*container_type);
            if (type_base(*container_type) == "array" && arguments.size() == 1)
                return arguments.front();
            if (type_base(*container_type) == "dict" && arguments.size() == 2)
                return arguments.back();
        }
    }

    if (auto var = std::dynamic_pointer_cast<Variable>(expr))
    {
        int slot = resolve_local(var->name.lexeme);

        if (slot != -1)
        {
            auto &local = locals[slot];

            if (local.type_name)
                return local.type_name;
        }

        auto it = global_types.find(var->name.lexeme);

        if (it != global_types.end())
            return it->second;
    }

    if (auto ternary = std::dynamic_pointer_cast<Ternary>(expr))
    {
        auto true_type = infer_expression_type(ternary->true_expression);
        auto false_type = infer_expression_type(ternary->false_expression);
        if (true_type && false_type && type_matches(*true_type, *false_type) &&
            type_matches(*false_type, *true_type))
            return true_type;
        return std::nullopt;
    }

    if (auto get = std::dynamic_pointer_cast<Get>(expr))
    {
        auto receiver_type = infer_expression_type(get->object);
        if (receiver_type)
        {
            std::string class_name = *receiver_type;
            while (class_properties.contains(class_name))
            {
                auto property = class_properties[class_name].find(get->name.lexeme);
                if (property != class_properties[class_name].end() && property->second.type_name)
                    return property->second.type_name;

                auto parent = class_superclasses.find(class_name);
                if (parent == class_superclasses.end())
                    break;
                class_name = parent->second;
            }
        }
    }

    if (std::dynamic_pointer_cast<This>(expr) && current_class_name)
        return current_class_name;

    if (auto unary = std::dynamic_pointer_cast<Unary>(expr))
    {
        switch (unary->operator_token.type)
        {
        case MINUS:
        case PLUS_PLUS:
        case MINUS_MINUS:
            return "number";

        case BANG:
            return "bool";

        default:
            break;
        }
    }

    if (auto binary = std::dynamic_pointer_cast<Binary>(expr))
    {
        auto left = infer_expression_type(binary->left);
        auto right = infer_expression_type(binary->right);

        switch (binary->operator_token.type)
        {
        case PLUS:
            if (!left || !right)
                return std::nullopt;

            if (*left == "string" && *right == "string")
                return "string";
            if (*left == "array" && *right == "array")
                return "array";
            if (*left == "number" && *right == "number")
                return "number";

            return std::nullopt;

        case MINUS:
        case STAR:
        case SLASH:
        case PERCENT:
        case STAR_STAR:
            return "number";

        case GREATER:
        case GREATER_EQUAL:
        case LESS:
        case LESS_EQUAL:
        case EQUAL_EQUAL:
        case BANG_EQUAL:
            return "bool";

        default:
            break;
        }
    }

    if (auto call =
            std::dynamic_pointer_cast<Call>(expr))
    {
        if (auto var =
                std::dynamic_pointer_cast<Variable>(
                    call->callee))
        {
            auto it =
                function_signatures.find(
                    var->name.lexeme);

            if (it != function_signatures.end())
            {
                return it->second->return_type;
            }

            // This is the best idea that came to mind at the moment of developing this feature
            // Mapping the return types directly saves a lot of effort

            static const std::unordered_map<std::string, std::string> native_return_types = {
                {"scan", "string"},
                {"format", "string"},
                {"print", "null"},
                {"println", "null"},
                {"printf", "null"},
                {"led_on", "null"},
                {"led_off", "null"},
                {"led_toggle", "null"},
                {"exit", "null"},
                {"methods", "array"},
                {"construct", "tag"},
                {"a", "tag"}, {"address", "tag"}, {"area", "tag"},
                {"article", "tag"}, {"aside", "tag"}, {"audio", "tag"},
                {"blockquote", "tag"}, {"body", "tag"}, {"br", "tag"},
                {"button", "tag"}, {"canvas", "tag"}, {"caption", "tag"},
                {"cite", "tag"}, {"code", "tag"}, {"col", "tag"},
                {"colgroup", "tag"}, {"datalist", "tag"}, {"dd", "tag"},
                {"del", "tag"}, {"div", "tag"}, {"dl", "tag"}, {"dt", "tag"},
                {"em", "tag"}, {"fieldset", "tag"}, {"figcaption", "tag"},
                {"figure", "tag"}, {"footer", "tag"}, {"form", "tag"},
                {"h1", "tag"}, {"h2", "tag"}, {"h3", "tag"}, {"h4", "tag"},
                {"h5", "tag"}, {"h6", "tag"}, {"head", "tag"},
                {"header", "tag"}, {"hr", "tag"}, {"iframe", "tag"},
                {"img", "tag"}, {"input", "tag"}, {"ins", "tag"},
                {"label", "tag"}, {"legend", "tag"}, {"li", "tag"},
                {"link", "tag"}, {"main", "tag"}, {"map", "tag"},
                {"mark", "tag"}, {"meta", "tag"}, {"meter", "tag"},
                {"nav", "tag"}, {"object", "tag"}, {"ol", "tag"},
                {"optgroup", "tag"}, {"option", "tag"}, {"output", "tag"},
                {"p", "tag"}, {"param", "tag"}, {"picture", "tag"},
                {"pre", "tag"}, {"progress", "tag"}, {"q", "tag"},
                {"section", "tag"}, {"select", "tag"}, {"small", "tag"},
                {"source", "tag"}, {"span", "tag"}, {"strong", "tag"},
                {"style", "tag"}, {"sub", "tag"}, {"sup", "tag"},
                {"table", "tag"}, {"tbody", "tag"}, {"td", "tag"},
                {"textarea", "tag"}, {"tfoot", "tag"}, {"th", "tag"},
                {"thead", "tag"}, {"time", "tag"}, {"title", "tag"},
                {"tr", "tag"}, {"track", "tag"}, {"ul", "tag"},
                {"video", "tag"}, {"wbr", "tag"}};
            const auto native_type = native_return_types.find(var->name.lexeme);
            if (native_type != native_return_types.end())
                return native_type->second;
        }

        if (auto var = std::dynamic_pointer_cast<Variable>(call->callee))
            if (class_signatures.contains(var->name.lexeme))
                return var->name.lexeme;

        if (auto get = std::dynamic_pointer_cast<Get>(call->callee))
        {
            auto receiver_type = infer_expression_type(get->object);
            std::string class_name = receiver_type.value_or("");
            if (get->name.lexeme == "type")
                return "string";

            static const std::unordered_map<std::string, std::string> module_methods = {
                {"regex.match", "array"},
                {"timing.date", "dict"}, {"timing.second_to_minute", "number"},
                {"timing.second_to_hour", "number"}, {"timing.second_to_day", "number"},
                {"timing.minute_to_second", "number"}, {"timing.minute_to_hour", "number"},
                {"timing.minute_to_day", "number"}, {"timing.hour_to_second", "number"},
                {"timing.hour_to_minute", "number"}, {"timing.hour_to_day", "number"},
                {"timing.leap_year", "bool"}, {"timing.today", "string"},
                {"encode.base", "string"},
                {"math.pow", "number"}, {"math.sqrt", "number"}, {"math.fact", "number"},
                {"math.floor", "number"}, {"math.ceil", "number"}, {"math.abs", "number"},
                {"math.log", "number"}, {"math.log10", "number"}, {"math.log2", "number"},
                {"math.max", "number"}, {"math.min", "number"}, {"math.average", "number"},
                {"math.sin", "number"}, {"math.cos", "number"}, {"math.tan", "number"},
                {"math.asin", "number"}, {"math.acos", "number"}, {"math.atan", "number"},
                {"math.degrees", "number"}, {"math.radians", "number"},
                {"math.is_odd", "bool"}, {"math.is_even", "bool"}, {"math.is_prime", "bool"},
                {"math.gcd", "number"}, {"math.lcm", "number"}, {"math.next_prime", "number"},
                {"math.sum_digits", "number"}, {"math.fibonacci", "number"},
                {"math.palindrome", "bool"}, {"math.round", "number"}, {"math.pi", "number"},
                {"math.precision", "number"},
                {"text.parse_json", "any"}, {"text.stringify", "string"},
                {"text.to_json", "string"}, {"text.parse_csv", "array"},
                {"text.to_csv", "string"},
                {"random.double", "number"}, {"random.integer", "number"},
                {"random.rgb_color", "array"}, {"random.hex_color", "string"},
                {"array.fill", "array"}, {"array.init", "array"}, {"array.input", "array"},





                {"profiler.start", "null"}, {"profiler.end", "null"},
                {"profiler.report", "string"}, {"profiler.reset", "null"},
            };


            if (auto module = std::dynamic_pointer_cast<Variable>(get->object))
            {
                const auto native_type = module_methods.find(module->name.lexeme + "." + get->name.lexeme);
                if (native_type != module_methods.end())
                    return native_type->second;
            }

            static const std::unordered_map<std::string, std::string> value_methods = {
                {"array.push", "bool"}, {"array.pop", "any"}, {"array.length", "number"},
                {"array.is_empty", "bool"}, {"array.clear", "bool"},
                {"array.contains", "bool"},
                {"array.first_index", "number"}, {"array.join", "string"},
                {"array.insert", "bool"}, {"array.remove", "any"},
                {"array.reverse", "bool"},
                {"array.first", "any"}, {"array.last", "any"}, {"array.slice", "array[any]"},
                {"array.copy", "array[any]"}, {"array.concat", "array[any]"}, {"array.swap", "bool"},
                {"array.count", "number"}, {"array.equals", "bool"},
                {"array.last_index", "number"}, {"array.sum", "number"},
                {"array.min", "number"}, {"array.max", "number"},
                {"array.average", "number"}, {"array.trim", "bool"},
                {"string.type", "string"}, {"string.to_string", "string"},
                {"string.to_number", "number"}, {"string.to_array", "array"},
                {"string.upper", "string"}, {"string.lower", "string"},
                {"string.capitalize", "string"}, {"string.swap", "string"},
                {"string.find", "number"}, {"string.find_last", "number"},
                {"string.first", "any"}, {"string.last", "any"},
                {"string.starts_with", "bool"}, {"string.ends_with", "bool"},
                {"string.is_number", "bool"}, {"string.trim", "string"},
                {"string.replace", "string"},
                {"string.is_space", "bool"}, {"string.is_all_spaces", "bool"},
                {"string.is_empty", "bool"},
                {"string.length", "number"}, {"string.slice", "string"},
                {"string.count", "number"},
                {"number.type", "string"}, {"number.to_string", "string"},
                {"number.to_int", "number"}, {"bool.type", "string"},
                {"bool.to_string", "string"}, {"bool.to_int", "number"},
                {"array.type", "string"}, {"dict.type", "string"},
                {"dict.contains", "bool"},
                {"dict.remove", "bool"}, {"dict.length", "number"},
                {"dict.keys", "array"}, {"dict.values", "array"},
                {"dict.pop", "any"}, {"dict.copy", "dict"}, {"dict.clear", "bool"},
                {"tag.attr", "tag"}, {"tag.append", "tag"}};
            if (receiver_type)
            {
                const auto method_type = value_methods.find(type_base(*receiver_type) + "." + get->name.lexeme);
                if (method_type != value_methods.end())
                    return method_type->second;
            }

            while (class_signatures.contains(class_name))
            {
                auto method = class_signatures[class_name].find(get->name.lexeme);
                if (method != class_signatures[class_name].end())
                    return method->second->return_type;
                auto parent = class_superclasses.find(class_name);
                if (parent == class_superclasses.end())
                    break;
                class_name = parent->second;
            }
        }
    }

    return std::nullopt;
}

std::optional<Value> Compiler::try_fold_constant(std::shared_ptr<Expression> expr)
{
    if (auto literal = std::dynamic_pointer_cast<Literal>(expr))
        return literal->value;
    if (auto grouping = std::dynamic_pointer_cast<Grouping>(expr))
        return try_fold_constant(grouping->expression);
    if (auto unary = std::dynamic_pointer_cast<Unary>(expr))
    {
        auto operand = try_fold_constant(unary->right);
        if (!operand)
            return std::nullopt;
        if (unary->operator_token.type == MINUS)
            if (auto number = value_get_if<double>(&*operand))
                return Value{-*number};
        if (unary->operator_token.type == BANG)
        {
            if (value_is<std::nullptr_t>(*operand)) return Value{true};
            if (auto boolean = value_get_if<bool>(&*operand)) return Value{!*boolean};
            return Value{false};
        }
    }
    if (auto binary = std::dynamic_pointer_cast<Binary>(expr))
    {
        auto left = try_fold_constant(binary->left);
        auto right = try_fold_constant(binary->right);
        if (!left || !right)
            return std::nullopt;
        auto a = value_get_if<double>(&*left);
        auto b = value_get_if<double>(&*right);
        if (a && b)
        {
            switch (binary->operator_token.type)
            {
            case PLUS: return Value{*a + *b};
            case MINUS: return Value{*a - *b};
            case STAR: return Value{*a * *b};
            case SLASH: if (*b != 0) return Value{*a / *b}; return std::nullopt;
            case PERCENT: if (*b != 0) return Value{std::fmod(*a, *b)}; return std::nullopt;
            case STAR_STAR: return Value{std::pow(*a, *b)};
            case GREATER: return Value{*a > *b};
            case GREATER_EQUAL: return Value{*a >= *b};
            case LESS: return Value{*a < *b};
            case LESS_EQUAL: return Value{*a <= *b};
            case EQUAL_EQUAL: return Value{*a == *b};
            case BANG_EQUAL: return Value{*a != *b};
            default: break;
            }
        }
        if (binary->operator_token.type == PLUS &&
            value_is<std::string>(*left) && value_is<std::string>(*right))
            return Value{value_get<std::string>(*left) + value_get<std::string>(*right)};
        if (binary->operator_token.type == EQUAL_EQUAL || binary->operator_token.type == BANG_EQUAL)
        {
            bool equal = *left == *right;
            return Value{binary->operator_token.type == EQUAL_EQUAL ? equal : !equal};
        }
    }
    return std::nullopt;
}

std::shared_ptr<Function> Compiler::compile_function(FunctionStmt &stmt, bool is_constructor, bool is_method, bool has_super_class, std::optional<std::string> super_class_name, std::optional<std::string> class_name)
{
    if (stmt.params.size() > UINT8_MAX)
        throw std::runtime_error("A function cannot have more than 255 parameters");

    const std::string function_owner_class = class_name.value_or("");
    Compiler function_compiler;
    function_compiler.global_types = global_types;
    function_compiler.function_signatures = function_signatures;
    function_compiler.class_signatures = class_signatures;
    function_compiler.class_superclasses = class_superclasses;
    function_compiler.class_properties = class_properties;
    function_compiler.in_function = true;
    function_compiler.in_method = is_method;
    function_compiler.current_method_name = is_method
        ? std::optional<std::string>(stmt.name.lexeme)
        : std::nullopt;
    function_compiler.current_method_is_public = stmt.is_public;
    function_compiler.in_subclass_method = is_method && has_super_class;
    function_compiler.current_super_class_name = std::move(super_class_name);
    function_compiler.current_class_name = class_name;
    function_compiler.current_return_type = stmt.return_type;
    function_compiler.begin_scope();

    if (is_method)
        function_compiler.locals.push_back({"this", 0, true, std::nullopt});

    std::vector<std::optional<std::string>> param_types;
    int slot = function_compiler.locals.size();
    for (auto &param : stmt.params)
    {
        if (function_compiler.locals.size() > UINT8_MAX)
            throw std::runtime_error("A function cannot use more than 256 local slots");
        function_compiler.locals.push_back({param.name.lexeme,
                                            0,
                                            false,
                                            param.type_name});
        param_types.push_back(param.type_name);
        slot++;
    }

    for (auto &s : stmt.body)
        s->accept(function_compiler);

    if (is_method)
        class_properties = std::move(function_compiler.class_properties);

    if (!is_constructor)
        function_compiler.emit(OpCode::NULL);
    else
        function_compiler.emit_constant(IgnoreReturnValue{});

    function_compiler.emit(OpCode::RETURN);

    auto function = std::make_shared<Function>(
        stmt.name.lexeme,
        stmt.params.size(),
        function_compiler.chunk,
        param_types,
        stmt.return_type);
    function->owner_class = function_owner_class;
    function->is_public = stmt.is_public;
    return function;
}

void Compiler::patch_jump(int offset)
{
    int jump = chunk.code.size() - offset - 2;
    if (jump > UINT16_MAX)
        throw std::runtime_error("Jump too large");

    chunk.code[offset] = (jump >> 8) & 0xff;
    chunk.code[offset + 1] = jump & 0xff;
}

void Compiler::emit_loop(int loop_start)
{
    int offset = chunk.code.size() - loop_start + 3;
    emit(OpCode::LOOP);
    if (offset > UINT16_MAX)
        throw std::runtime_error("Loop too large");
    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

Value Compiler::visit_literal_expression(std::shared_ptr<Literal> expr)
{
    if (value_is<std::nullptr_t>(expr->value))
        emit(OpCode::NULL);
    else if (value_is<bool>(expr->value))
        emit(value_get<bool>(expr->value) ? OpCode::TRUE : OpCode::FALSE);
    else
        emit_constant(expr->value);

    return {};
}

Value Compiler::visit_grouping_expression(std::shared_ptr<Grouping> expr)
{
    expr->expression->accept(*this);
    return {};
}

Value Compiler::visit_unary_expression(std::shared_ptr<Unary> expr)
{
    if (expr->operator_token.type == PLUS_PLUS ||
        expr->operator_token.type == MINUS_MINUS)
    {
        const auto target_type = infer_expression_type(expr->right);
        if (target_type && !type_matches("number", *target_type))
            throw std::runtime_error("Increment and decrement require a number, got \"" + *target_type + "\"");

        const uint8_t direction = expr->operator_token.type == PLUS_PLUS ? 1 : 0;
        const uint8_t result_mode = expr->postfix ? 2 : 1;

        if (auto variable = std::dynamic_pointer_cast<Variable>(expr->right))
        {
            const int slot = in_function ? resolve_local(variable->name.lexeme) : -1;
            if (slot != -1)
            {
                const auto &local = locals[slot];
                if (local.is_const)
                    throw std::runtime_error("Cannot reassign const variable \"" + variable->name.lexeme + "\"");
                emit(OpCode::UPDATE_LOCAL);
                emit_byte(slot);
            }
            else
            {
                auto global_type = global_types.find(variable->name.lexeme);
                if (global_type != global_types.end() &&
                    !type_matches("number", global_type->second))
                    throw std::runtime_error("Increment and decrement require a number, got \"" + global_type->second + "\"");
                emit(OpCode::UPDATE_GLOBAL);
                emit_index(chunk.add_constant(variable->name.lexeme));
            }
            emit_byte(direction);
            emit_byte(result_mode);
        }
        else if (auto property = std::dynamic_pointer_cast<Get>(expr->right))
        {
            property->object->accept(*this);
            emit(OpCode::UPDATE_PROPERTY);
            emit_index(chunk.add_constant(property->name.lexeme));
            emit_byte(direction);
            emit_byte(result_mode);
        }
        else if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr->right))
        {
            index->array->accept(*this);
            index->index->accept(*this);
            emit(OpCode::UPDATE_INDEX);
            emit_byte(direction);
            emit_byte(result_mode);
        }
        return {};
    }

    expr->right->accept(*this);
    switch (expr->operator_token.type)
    {
    case MINUS:
        emit(OpCode::NEGATE);
        break;
    case BANG:
        emit(OpCode::NOT);
        break;

    default:
        throw std::runtime_error("Invalid unary operator");
    }

    return {};
}

Value Compiler::visit_binary_expression(std::shared_ptr<Binary> expr)
{
    expr->left->accept(*this);
    expr->right->accept(*this);

    switch (expr->operator_token.type)
    {
    case PLUS:
        emit(OpCode::ADD);
        break;
    case MINUS:
        emit(OpCode::SUBTRACT);
        break;
    case STAR:
        emit(OpCode::MULTIPLY);
        break;
    case STAR_STAR:
        emit(OpCode::POWER);
        break;
    case SLASH:
        emit(OpCode::DIVIDE);
        break;
    case PERCENT:
        emit(OpCode::PERCENT);
        break;

    case EQUAL_EQUAL:
        emit(OpCode::EQUAL);
        break;
    case BANG_EQUAL:
        emit(OpCode::NOT_EQUAL);
        break;
    case GREATER:
        emit(OpCode::GREATER);
        break;
    case GREATER_EQUAL:
        emit(OpCode::GREATER_EQUAL);
        break;
    case LESS:
        emit(OpCode::LESS);
        break;
    case LESS_EQUAL:
        emit(OpCode::LESS_EQUAL);
        break;

    default:
        throw std::runtime_error("Unsupported binary operator");
    }

    return {};
}

Value Compiler::visit_expression_stmt(std::shared_ptr<ExprStmt> stmt)
{
    if (std::dynamic_pointer_cast<Assign>(stmt->expression))
        discard_assignment_result = true;

    stmt->expression->accept(*this);

    if (assignment_result_was_discarded)
        assignment_result_was_discarded = false;
    else
        emit(OpCode::POP);

    return {};
}

Value Compiler::visit_var_stmt(std::shared_ptr<VarStmt> stmt)
{
    auto initializer_type = stmt->initializer ? infer_expression_type(stmt->initializer) : std::nullopt;

    if (scope_depth > 0 && in_function)
    {
        int slot_index = locals.size();
        if (slot_index > UINT8_MAX)
            throw std::runtime_error("A function cannot use more than 256 local slots");
        locals.push_back({stmt->name.lexeme,
                          scope_depth,
                          stmt->is_const,
                          stmt->type_name});

        if (stmt->type_name && stmt->initializer)
        {
            if (!initializer_type)
            {
                throw std::runtime_error(
                    "Cannot infer type for initializer of \"" +
                    stmt->name.lexeme + "\"");
            }

            if (!type_matches_expression(*stmt->type_name, *initializer_type, stmt->initializer))
            {
                throw std::runtime_error(
                    "Type mismatch for variable '" + stmt->name.lexeme +
                    "': declared '" + *stmt->type_name +
                    "', but initializer is '" + *initializer_type + "'");
            }
        }

        if (stmt->initializer)
            stmt->initializer->accept(*this);
        else
            emit(OpCode::NULL);

        emit(OpCode::SET_LOCAL);
        emit_byte(slot_index);
    }
    else
    {
        if (stmt->initializer)
        {
            if (stmt->type_name)
            {
                if (!initializer_type)
                    throw std::runtime_error(
                        "Cannot infer type for initializer of global \"" +
                        stmt->name.lexeme + "\"");

                if (!type_matches_expression(*stmt->type_name, *initializer_type, stmt->initializer))
                    throw std::runtime_error(
                        "Type mismatch for global variable '" + stmt->name.lexeme +
                        "': declared '" + *stmt->type_name +
                        "', but initializer is '" + *initializer_type + "'");
            }

            stmt->initializer->accept(*this);
        }
        else
        {
            emit(OpCode::NULL);
        }

        if (stmt->type_name)
            global_types[stmt->name.lexeme] = *stmt->type_name;

        uint16_t name = chunk.add_constant(stmt->name.lexeme);

        if (stmt->is_const)
            emit(OpCode::DEFINE_CONST);
        else
        {
            emit(OpCode::DEFINE_GLOBAL);
        }
        emit_index(name);
    }

    return {};
}

Value Compiler::visit_if_stmt(std::shared_ptr<IfStmt> stmt)
{
    stmt->condition->accept(*this);
    int else_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    stmt->then_branch->accept(*this);

    int end_jump = 0;
    if (stmt->else_branch)
    {
        end_jump = emit_jump(OpCode::JUMP);
    }

    patch_jump(else_jump);

    if (stmt->else_branch)
    {
        stmt->else_branch->accept(*this);
        patch_jump(end_jump);
    }

    return {};
}

Value Compiler::visit_while_stmt(std::shared_ptr<WhileStmt> stmt)
{
    const int loop_start = static_cast<int>(chunk.code.size());
    auto condition = std::dynamic_pointer_cast<Binary>(stmt->condition);
    auto condition_variable = condition ? std::dynamic_pointer_cast<Variable>(condition->left) : nullptr;
    auto condition_limit = condition ? std::dynamic_pointer_cast<Literal>(condition->right) : nullptr;
    const bool is_numeric_comparison = condition && condition_variable && condition_limit &&
        value_is<double>(condition_limit->value) &&
        (condition->operator_token.type == LESS ||
         condition->operator_token.type == LESS_EQUAL ||
         condition->operator_token.type == GREATER ||
         condition->operator_token.type == GREATER_EQUAL);

    if (is_numeric_comparison)
    {
        std::shared_ptr<BlockStmt> body_block =
            std::dynamic_pointer_cast<BlockStmt>(stmt->body);
        std::shared_ptr<Stmt> increment_statement = stmt->body;
        if (body_block && !body_block->statements.empty())
            increment_statement = body_block->statements.back();

        auto increment_expression_statement =
            std::dynamic_pointer_cast<ExprStmt>(increment_statement);
        uint8_t loop_update_mode = 0;
        auto increment_name = [&]() -> std::optional<std::string>
        {
            if (!increment_expression_statement)
                return std::nullopt;

            auto expression = increment_expression_statement->expression;
            if (auto assignment = std::dynamic_pointer_cast<Assign>(expression))
            {
                auto binary = std::dynamic_pointer_cast<Binary>(assignment->value);
                auto left = binary ? std::dynamic_pointer_cast<Variable>(binary->left) : nullptr;
                auto right = binary ? std::dynamic_pointer_cast<Literal>(binary->right) : nullptr;
                if (binary && binary->operator_token.type == PLUS && left && right &&
                    left->name.lexeme == assignment->name.lexeme &&
                    value_is<double>(right->value) &&
                    value_get<double>(right->value) == 1.0)
                {
                    loop_update_mode = 0;
                    return assignment->name.lexeme;
                }
            }
            else if (auto unary = std::dynamic_pointer_cast<Unary>(expression))
            {
                auto variable = std::dynamic_pointer_cast<Variable>(unary->right);
                if (variable && unary->operator_token.type == PLUS_PLUS)
                {
                    loop_update_mode = 1;
                    return variable->name.lexeme;
                }
                if (variable && unary->operator_token.type == MINUS_MINUS)
                {
                    loop_update_mode = 2;
                    return variable->name.lexeme;
                }
            }
            return std::nullopt;
        }();

        bool fuse_increment = increment_name &&
            *increment_name == condition_variable->name.lexeme;
        if (fuse_increment && body_block)
        {
            for (size_t i = 0; i + 1 < body_block->statements.size(); ++i)
            {
                auto declaration = std::dynamic_pointer_cast<VarStmt>(body_block->statements[i]);
                if (declaration && declaration->name.lexeme == *increment_name)
                {
                    fuse_increment = false;
                    break;
                }
            }
        }
        const int slot = in_function ? resolve_local(condition_variable->name.lexeme) : -1;
        if (fuse_increment && slot != -1 && locals[slot].is_const)
            fuse_increment = false;
        auto increment_assignment = increment_expression_statement
            ? std::dynamic_pointer_cast<Assign>(increment_expression_statement->expression)
            : nullptr;
        if (fuse_increment && increment_assignment)
        {
            const auto assigned_type = infer_expression_type(increment_assignment->value);
            if (slot != -1 && locals[slot].type_name && assigned_type &&
                !type_matches_expression(*locals[slot].type_name, *assigned_type, increment_assignment->value))
                fuse_increment = false;
            else if (slot == -1)
            {
                auto global_type = global_types.find(increment_assignment->name.lexeme);
                if (global_type != global_types.end() && assigned_type &&
                    !type_matches_expression(global_type->second, *assigned_type, increment_assignment->value))
                    fuse_increment = false;
            }
        }

        bool body_has_no_work = true;
        if (body_block)
        {
            for (size_t i = 0; i + 1 < body_block->statements.size(); ++i)
            {
                auto empty_block = std::dynamic_pointer_cast<BlockStmt>(body_block->statements[i]);
                if (!empty_block || !empty_block->statements.empty())
                {
                    body_has_no_work = false;
                    break;
                }
            }
        }

        const auto counter_type = slot != -1 && locals[slot].type_name
            ? locals[slot].type_name
            : (slot == -1 && global_types.contains(condition_variable->name.lexeme)
                   ? std::optional<std::string>(global_types.at(condition_variable->name.lexeme))
                   : std::nullopt);
        const bool fast_forward_loop = fuse_increment && body_has_no_work &&
            loop_update_mode != 0 && (!counter_type || *counter_type == "number") &&
            (slot == -1 || !locals[slot].is_const);

        if (fast_forward_loop)
        {
            const uint16_t limit_index = chunk.add_constant(condition_limit->value);
            if (slot == -1)
            {
                emit(OpCode::FAST_FORWARD_GLOBAL_LOOP);
                emit_index(chunk.add_constant(condition_variable->name.lexeme));
            }
            else
            {
                emit(OpCode::FAST_FORWARD_LOCAL_LOOP);
                emit_byte(slot);
            }
            emit_index(limit_index);
            emit_byte(static_cast<size_t>(condition->operator_token.type));
            emit_byte(loop_update_mode);
            return {};
        }

        if (slot == -1)
        {
            const uint16_t name_index = chunk.add_constant(condition_variable->name.lexeme);
            emit(OpCode::JUMP_IF_GLOBAL_COMPARE);
            emit_index(name_index);
        }
        else
        {
            emit(OpCode::JUMP_IF_LOCAL_COMPARE);
            emit_byte(slot);
        }

        const uint16_t limit_index = chunk.add_constant(condition_limit->value);
        emit_index(limit_index);
        emit_byte(static_cast<size_t>(condition->operator_token.type));
        const int exit_jump = chunk.code.size();
        emit_byte(0xff);
        emit_byte(0xff);
        const int loop_body_start = static_cast<int>(chunk.code.size());

        if (fuse_increment)
        {
            if (body_block)
            {
                begin_scope();
                for (size_t i = 0; i + 1 < body_block->statements.size(); ++i)
                    body_block->statements[i]->accept(*this);
                end_scope();
            }

            const int counter_slot = in_function ? resolve_local(*increment_name) : -1;
            const bool local_counter = counter_slot != -1;
            emit(local_counter ? OpCode::LOOP_LOCAL_INCREMENT_COMPARE
                               : OpCode::LOOP_GLOBAL_INCREMENT_COMPARE);
            if (local_counter)
                emit_byte(counter_slot);
            else
                emit_index(chunk.add_constant(*increment_name));
            emit_index(limit_index);
            emit_byte(static_cast<size_t>(condition->operator_token.type));
            emit_byte(loop_update_mode);

            const int offset = static_cast<int>(chunk.code.size()) + 2 - loop_body_start;
            if (offset > UINT16_MAX)
                throw std::runtime_error("Loop too large");
            emit_byte((offset >> 8) & 0xff);
            emit_byte(offset & 0xff);
        }
        else
        {
            stmt->body->accept(*this);
            emit_loop(loop_start);
        }
        patch_jump(exit_jump);
        return {};
    }

    stmt->condition->accept(*this);
    int exit_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    stmt->body->accept(*this);
    emit_loop(loop_start);
    patch_jump(exit_jump);

    return {};
}

Value Compiler::visit_logical_expression(std::shared_ptr<Logical> expr)
{
    expr->left->accept(*this);

    if (expr->operator_token.type == OR)
    {
        int jump = emit_jump(OpCode::JUMP_IF_TRUE);
        expr->right->accept(*this);

        int end_jump = emit_jump(OpCode::JUMP);

        patch_jump(jump);
        emit(OpCode::TRUE);

        patch_jump(end_jump);
    }
    else
    {
        int jump = emit_jump(OpCode::JUMP_IF_FALSE);
        expr->right->accept(*this);

        int end_jump = emit_jump(OpCode::JUMP);

        patch_jump(jump);
        emit(OpCode::FALSE);

        patch_jump(end_jump);
    }

    return {};
}

Value Compiler::visit_ternary_expression(std::shared_ptr<Ternary> expr)
{
    expr->condition->accept(*this);
    const int false_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    expr->true_expression->accept(*this);
    const int end_jump = emit_jump(OpCode::JUMP);
    patch_jump(false_jump);
    expr->false_expression->accept(*this);
    patch_jump(end_jump);
    return {};
}

Value Compiler::visit_assign_expression(std::shared_ptr<Assign> expr)
{
    const bool discard_result = std::exchange(discard_assignment_result, false);
    auto inferred = infer_expression_type(expr->value);

    int slot = in_function ? resolve_local(expr->name.lexeme) : -1;

    if (slot != -1)
    {
        auto &local = locals[slot];

        if (local.is_const)
        {
            throw std::runtime_error(
                "Cannot reassign const variable \"" +
                expr->name.lexeme + "\"");
        }

        if (local.type_name && inferred)
        {
            if (!type_matches_expression(*local.type_name, *inferred, expr->value))
            {
                throw std::runtime_error(
                    "Cannot assign " +
                    *inferred +
                    " to variable \"" +
                    expr->name.lexeme +
                    "\" of type " +
                    *local.type_name);
            }
        }

        auto binary = std::dynamic_pointer_cast<Binary>(expr->value);
        auto left = binary ? std::dynamic_pointer_cast<Variable>(binary->left) : nullptr;
        auto right = binary ? std::dynamic_pointer_cast<Literal>(binary->right) : nullptr;
        if (binary && binary->operator_token.type == PLUS && left && right &&
            left->name.lexeme == expr->name.lexeme &&
            value_is<double>(right->value) && value_get<double>(right->value) == 1.0)
        {
            emit(OpCode::INCREMENT_LOCAL);
            emit_byte(slot);
            emit_byte(discard_result ? 0 : 1);
            assignment_result_was_discarded = discard_result;
            return {};
        }

        expr->value->accept(*this);

        emit(OpCode::SET_LOCAL);
        emit_byte(slot);

        return {};
    }

    auto it = global_types.find(expr->name.lexeme);

    if (it != global_types.end() && inferred)
    {
        if (it->second != *inferred)
        {
            throw std::runtime_error(
                "Cannot assign " +
                *inferred +
                " to variable \"" +
                expr->name.lexeme +
                "\" of type " +
                it->second);
        }
    }

    auto binary = std::dynamic_pointer_cast<Binary>(expr->value);
    auto left = binary ? std::dynamic_pointer_cast<Variable>(binary->left) : nullptr;
    auto right = binary ? std::dynamic_pointer_cast<Literal>(binary->right) : nullptr;
    if (binary && binary->operator_token.type == PLUS && left && right &&
        left->name.lexeme == expr->name.lexeme &&
        value_is<double>(right->value) && value_get<double>(right->value) == 1.0)
    {
        uint16_t name = chunk.add_constant(expr->name.lexeme);
        emit(OpCode::INCREMENT_GLOBAL);
        emit_index(name);
        emit_byte(discard_result ? 0 : 1);
        assignment_result_was_discarded = discard_result;
        return {};
    }

    expr->value->accept(*this);

    uint16_t name = chunk.add_constant(expr->name.lexeme);

    emit(OpCode::SET_GLOBAL);
    emit_index(name);

    return {};
}

Value Compiler::visit_variable_expression(std::shared_ptr<Variable> expr)
{
    int slot = in_function ? resolve_local(expr->name.lexeme) : -1;
    if (slot != -1)
    {
        emit(OpCode::GET_LOCAL);
        emit_byte(slot);
    }
    else
    {
        uint16_t name = chunk.add_constant(expr->name.lexeme);
        emit(OpCode::GET_GLOBAL);
        emit_index(name);
    }
    return {};
}

Value Compiler::visit_block_stmt(std::shared_ptr<BlockStmt> stmt)
{
    begin_scope();
    for (auto &s : stmt->statements)
        s->accept(*this);
    end_scope();
    return {};
}

Value Compiler::visit_function_stmt(std::shared_ptr<FunctionStmt> stmt)
{
    auto fn = compile_function(*stmt, false, false);

    uint16_t fn_index = chunk.add_constant(fn);
    emit(OpCode::CLOSURE);
    emit_index(fn_index);

    uint16_t name_index = chunk.add_constant(stmt->name.lexeme);
    emit(OpCode::DEFINE_GLOBAL);
    emit_index(name_index);

    fn->param_types.clear();
    for (auto &param : stmt->params)
        fn->param_types.push_back(param.type_name ? *param.type_name : "any");

    fn->return_type = stmt->return_type;
    function_signatures[stmt->name.lexeme] = fn;

    return {};
}

Value Compiler::visit_call_expression(std::shared_ptr<Call> expr)
{
    expr->callee->accept(*this);

    std::shared_ptr<Function> callee_fn = nullptr;
    bool no_constructor = false;

    if (auto var = std::dynamic_pointer_cast<Variable>(expr->callee))
    {
        auto it = function_signatures.find(var->name.lexeme);
        if (it != function_signatures.end())
            callee_fn = it->second;
        else if (class_signatures.contains(var->name.lexeme))
        {
            std::string class_name = var->name.lexeme;
            while (class_signatures.contains(class_name))
            {
                auto init = class_signatures[class_name].find("init");
                if (init != class_signatures[class_name].end())
                {
                    callee_fn = init->second;
                    break;
                }
                auto parent = class_superclasses.find(class_name);
                if (parent == class_superclasses.end())
                    break;
                class_name = parent->second;
            }
            no_constructor = !callee_fn;
        }
    }

    if (!callee_fn)
    {
        if (auto get = std::dynamic_pointer_cast<Get>(expr->callee))
        {
            std::string class_name = infer_expression_type(get->object).value_or("");
            while (class_signatures.contains(class_name))
            {
                auto method = class_signatures[class_name].find(get->name.lexeme);
                if (method != class_signatures[class_name].end())
                {
                    callee_fn = method->second;
                    break;
                }
                auto parent = class_superclasses.find(class_name);
                if (parent == class_superclasses.end())
                    break;
                class_name = parent->second;
            }
        }
    }

    if (no_constructor && !expr->arguments.empty())
        throw std::runtime_error("Constructor expected 0 arguments but got " + std::to_string(expr->arguments.size()));

    if (callee_fn)
    {
        if (expr->arguments.size() != callee_fn->arity)
        {
            throw std::runtime_error(
                "Expected " + std::to_string(callee_fn->arity) +
                " arguments but got " + std::to_string(expr->arguments.size()));
        }

        for (size_t i = 0; i < expr->arguments.size(); i++)
        {
            auto arg_type = infer_expression_type(expr->arguments[i]);
            const auto &param_type = callee_fn->param_types[i];

            if (param_type && arg_type &&
                !type_matches_expression(*param_type, *arg_type, expr->arguments[i]))
            {
                throw std::runtime_error(
                    "Argument " + std::to_string(i + 1) +
                    " expected type \"" + (param_type ? *param_type : "any") + "\"" +
                    ", got \"" + (arg_type ? *arg_type : "unknown") + "\"");
            }
        }
    }

    if (auto get = std::dynamic_pointer_cast<Get>(expr->callee))
    {
        auto receiver_type = infer_expression_type(get->object);
        if (receiver_type && type_base(*receiver_type) == "array")
        {
            const auto arguments = type_arguments(*receiver_type);
            if (arguments.size() == 1)
            {
                std::optional<size_t> value_argument_start;
                if (get->name.lexeme == "push")
                    value_argument_start = 0;
                else if (get->name.lexeme == "insert")
                    value_argument_start = 1;
                else if (get->name.lexeme == "concat")
                    value_argument_start = 0;

                if (value_argument_start)
                {
                    for (size_t i = *value_argument_start; i < expr->arguments.size(); ++i)
                    {
                        auto argument_type = infer_expression_type(expr->arguments[i]);
                        const std::string expected_type = get->name.lexeme == "concat"
                                                              ? "array[" + arguments.front() + "]"
                                                              : arguments.front();
                        if (argument_type && !type_matches(expected_type, *argument_type))
                            throw std::runtime_error("Array method \"" + get->name.lexeme +
                                                     "\" expects elements of type \"" +
                                                     expected_type + "\", got \"" +
                                                     *argument_type + "\"");
                    }
                }
            }
        }
    }

    for (auto &arg : expr->arguments)
        arg->accept(*this);

    emit(OpCode::CALL);
    emit_byte(expr->arguments.size());

    return {};
}

Value Compiler::visit_return_stmt(std::shared_ptr<ReturnStmt> stmt)
{
    std::optional<std::string> actual_type;

    if (stmt->value)
    {
        stmt->value->accept(*this);
        actual_type = infer_expression_type(stmt->value);
    }
    else
    {
        emit(OpCode::NULL);
        actual_type = "nothing";
    }

    if (current_return_type)
    {
        if (actual_type &&
            !type_matches_expression(*current_return_type, *actual_type, stmt->value))
        {
            throw std::runtime_error(
                "Expected return type \"" +
                *current_return_type +
                "\", got \"" +
                *actual_type + "\"");
        }
    }

    emit(OpCode::RETURN);

    return {};
}

Value Compiler::visit_array_expression(std::shared_ptr<ArrayExpr> expr)
{
    for (auto &element : expr->elements)
        element->accept(*this);

    emit(OpCode::ARRAY);
    emit_byte(expr->elements.size());

    return {};
}

Value Compiler::visit_index_expression(std::shared_ptr<IndexExpr> expr)
{
    expr->array->accept(*this);
    expr->index->accept(*this);

    emit(OpCode::GET_INDEX);
    return {};
}

Value Compiler::visit_index_assign_expression(std::shared_ptr<IndexAssign> expr)
{
    auto container_type = infer_expression_type(expr->array);
    auto index_type = infer_expression_type(expr->index);
    auto value_type = infer_expression_type(expr->value);
    if (container_type && value_type)
    {
        const auto arguments = type_arguments(*container_type);
        std::optional<std::string> expected_type;
        if (type_base(*container_type) == "array" && arguments.size() == 1)
        {
            if (index_type && !type_matches("number", *index_type))
                throw std::runtime_error("Array index must be a number, got " + *index_type);
            expected_type = arguments.front();
        }
        else if (type_base(*container_type) == "dict" && arguments.size() == 2)
        {
            if (index_type && !type_matches(arguments.front(), *index_type))
                throw std::runtime_error("Dictionary key must have type \"" +
                                         arguments.front() + "\", got \"" + *index_type + "\"");
            expected_type = arguments.back();
        }

        if (expected_type && !type_matches(*expected_type, *value_type))
            throw std::runtime_error("Cannot assign " + *value_type +
                                     " to container value of type " + *expected_type);
    }

    expr->array->accept(*this);
    expr->index->accept(*this);
    expr->value->accept(*this);

    emit(OpCode::SET_INDEX);
    return {};
}

Value Compiler::visit_get_expression(std::shared_ptr<Get> expr)
{
    std::optional<std::string> receiver_class;
    if (std::dynamic_pointer_cast<This>(expr->object) && current_class_name)
        receiver_class = current_class_name;
    else
        receiver_class = infer_expression_type(expr->object);

    if (receiver_class)
    {
        const std::string caller_class = current_class_name.value_or("");
        std::string class_name = *receiver_class;
        while (true)
        {
            auto class_it = class_properties.find(class_name);
            if (class_it != class_properties.end())
            {
                auto property = class_it->second.find(expr->name.lexeme);
                if (property != class_it->second.end() && !property->second.is_public &&
                    property->second.owner_class != caller_class)
                    throw std::runtime_error("Cannot access private property \"" +
                                             expr->name.lexeme + "\"");
            }

            auto methods = class_signatures.find(class_name);
            if (methods != class_signatures.end())
            {
                auto method = methods->second.find(expr->name.lexeme);
                if (method != methods->second.end() && !method->second->is_public &&
                    method->second->owner_class != caller_class)
                    throw std::runtime_error("Cannot access private method \"" +
                                             expr->name.lexeme + "\"");
            }

            auto parent = class_superclasses.find(class_name);
            if (parent == class_superclasses.end())
                break;
            class_name = parent->second;
        }
    }

    expr->object->accept(*this);
    uint16_t name = chunk.add_constant(expr->name.lexeme);
    emit(OpCode::GET_PROPERTY);
    emit_index(name);
    return {};
}

Value Compiler::visit_set_expression(std::shared_ptr<Set> expr)
{
    if (std::dynamic_pointer_cast<This>(expr->object) && current_class_name)
    {
        auto &properties = class_properties[*current_class_name];
        auto property = properties.find(expr->name.lexeme);
        if (property == properties.end())
        {
            if (!current_method_name || *current_method_name != "init")
                throw std::runtime_error("Property \"" + expr->name.lexeme +
                                         "\" must be declared in init()");

            properties.emplace(expr->name.lexeme,
                               PropertyInfo{*current_class_name, true, std::nullopt});
            property = properties.find(expr->name.lexeme);
        }

        if (!property->second.is_public &&
            property->second.owner_class != *current_class_name)
            throw std::runtime_error("Cannot access private property \"" +
                                     expr->name.lexeme + "\"");

        if (property->second.type_name)
        {
            auto actual_type = infer_expression_type(expr->value);
            if (actual_type && !type_matches_expression(*property->second.type_name,
                                                        *actual_type, expr->value))
                throw std::runtime_error("Cannot assign " + *actual_type +
                                         " to property \"" + expr->name.lexeme +
                                         "\" of type " + *property->second.type_name);
        }
    }

    expr->object->accept(*this);
    expr->value->accept(*this);
    uint16_t name_index = chunk.add_constant(expr->name.lexeme);
    emit(OpCode::SET_PROPERTY);
    emit_index(name_index);
    return {};
}

Value Compiler::visit_class_stmt(std::shared_ptr<ClassStmt> stmt)
{
    bool has_super_class = stmt->super_class != nullptr;

    auto &properties = class_properties[stmt->name.lexeme];
    properties.clear();
    if (has_super_class)
    {
        auto parent = class_properties.find(stmt->super_class->name.lexeme);
        if (parent != class_properties.end())
            properties = parent->second;
    }

    auto &method_signatures = class_signatures[stmt->name.lexeme];
    for (const auto &method : stmt->methods)
    {
        std::vector<std::optional<std::string>> parameter_types;
        for (const auto &parameter : method->params)
            parameter_types.push_back(parameter.type_name);
        method_signatures[method->name.lexeme] = std::make_shared<Function>(
            method->name.lexeme, method->params.size(), Chunk{}, std::move(parameter_types), method->return_type);
        method_signatures[method->name.lexeme]->is_public = method->is_public;
        method_signatures[method->name.lexeme]->owner_class = stmt->name.lexeme;
    }
    if (has_super_class)
        class_superclasses[stmt->name.lexeme] = stmt->super_class->name.lexeme;

    if (has_super_class)
    {
        stmt->super_class->accept(*this);
    }

    emit(OpCode::CLASS);
    uint16_t name_index = chunk.add_constant(stmt->name.lexeme);
    emit_index(name_index);

    std::unordered_map<std::string, std::shared_ptr<Function>> compiled_methods;
    std::vector<std::shared_ptr<FunctionStmt>> methods_in_compile_order;
    auto constructor = std::find_if(stmt->methods.begin(), stmt->methods.end(),
                                    [](const auto &method) { return method->name.lexeme == "init"; });
    if (constructor != stmt->methods.end())
        methods_in_compile_order.push_back(*constructor);
    for (auto &method : stmt->methods)
        if (method->name.lexeme != "init")
            methods_in_compile_order.push_back(method);

    for (auto &method : methods_in_compile_order)
    {
        bool is_constructor = method->name.lexeme == "init";
        auto fn = compile_function(*method, is_constructor, true, has_super_class,
                                   has_super_class ? std::optional<std::string>{stmt->super_class->name.lexeme} : std::nullopt,
                                   stmt->name.lexeme);
        if (is_constructor)
            fn->declared_properties = class_properties[stmt->name.lexeme];
        compiled_methods[method->name.lexeme] = fn;

        uint16_t fn_index = chunk.add_constant(fn);
        emit(OpCode::CLOSURE);
        emit_index(fn_index);

        uint16_t method_index = chunk.add_constant(method->name.lexeme);
        emit(OpCode::METHOD);
        emit_index(method_index);
    }

    class_signatures[stmt->name.lexeme] = std::move(compiled_methods);

    if (has_super_class)
    {
        emit(OpCode::INHERIT);
    }

    emit(OpCode::DEFINE_GLOBAL);
    emit_index(name_index);

    return {};
}

Value Compiler::visit_property_stmt(std::shared_ptr<PropertyStmt> stmt)
{
    if (!in_method || !current_method_name || *current_method_name != "init" ||
        !current_class_name)
        throw std::runtime_error("Properties can only be declared in init()");

    auto inferred = infer_expression_type(stmt->initializer);
    if (stmt->type_name && inferred &&
        !type_matches_expression(*stmt->type_name, *inferred, stmt->initializer))
        throw std::runtime_error("Type mismatch for property \"" + stmt->name.lexeme +
                                 "\": declared \"" + *stmt->type_name +
                                 "\", but initializer is \"" + *inferred + "\"");

    auto &properties = class_properties[*current_class_name];
    auto existing = properties.find(stmt->name.lexeme);
    if (existing != properties.end() && existing->second.owner_class == *current_class_name &&
        existing->second.type_name)
        throw std::runtime_error("Property \"" + stmt->name.lexeme + "\" is already declared");

    properties.insert_or_assign(stmt->name.lexeme,
                                PropertyInfo{*current_class_name, stmt->is_public,
                                             stmt->type_name});

    emit(OpCode::GET_LOCAL);
    emit_byte(0);
    stmt->initializer->accept(*this);
    emit(OpCode::SET_PROPERTY);
    emit_index(chunk.add_constant(stmt->name.lexeme));
    emit(OpCode::POP);
    return {};
}

Value Compiler::visit_super_expression(std::shared_ptr<Super> expr)
{
    if (!in_subclass_method)
        throw std::runtime_error("'super' can only be used inside a subclass method");
    if (current_super_class_name)
    {
        auto parent_methods = class_signatures.find(*current_super_class_name);
        if (parent_methods != class_signatures.end())
        {
            auto method = parent_methods->second.find(expr->method.lexeme);
            if (method != parent_methods->second.end() && !method->second->is_public &&
                method->second->owner_class != current_class_name.value_or(""))
                throw std::runtime_error("Cannot access private method \"" +
                                         expr->method.lexeme + "\"");
        }
    }
    emit(OpCode::GET_SUPER);
    uint16_t name_index = chunk.add_constant(expr->method.lexeme);
    emit_index(name_index);
    return {};
}

Value Compiler::visit_this_expression(std::shared_ptr<This> expr)
{
    if (!in_method)
        throw std::runtime_error("'this' can only be used inside a class method");
    emit(OpCode::GET_LOCAL);
    emit_byte(0);
    return {};
}

Value Compiler::visit_dict_expression(std::shared_ptr<DictExpr> expr)
{
    emit(OpCode::DICT);

    for (auto &[key, value] : expr->entries)
    {
        key->accept(*this);
        value->accept(*this);
        emit(OpCode::SET_ITEM);
    }

    return {};
}