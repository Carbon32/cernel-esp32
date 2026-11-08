#define NATIVE_PRIMITIVE_TYPE                                        \
    case NativeMethod::TYPE:                                         \
    {                                                                \
        if (!args.empty())                                           \
            throw std::runtime_error("type() takes no arguments");   \
                                                                     \
        const Value &v = method->receiver;                           \
                                                                     \
        if (value_is<std::nullptr_t>(v))                             \
            push("null");                                            \
        else if (value_is<bool>(v))                                  \
            push("bool");                                            \
        else if (value_is<double>(v))                                \
            push("number");                                          \
        else if (value_is<std::string>(v))                           \
            push("string");                                          \
        else if (value_is<std::shared_ptr<Array>>(v))                \
            push("array");                                           \
        else if (value_is<std::shared_ptr<Function>>(v))             \
            push("function");                                        \
        else if (value_is<std::shared_ptr<Class>>(v))                \
        {                                                            \
            auto klass = value_get<std::shared_ptr<Class>>(v);       \
            push("<class " + klass->name + ">");                     \
        }                                                            \
        else if (value_is<std::shared_ptr<Instance>>(v))             \
        {                                                            \
            auto instance = value_get<std::shared_ptr<Instance>>(v); \
            push("<" + instance->klass->name + " instance>");        \
        }                                                            \
        else if (value_is<std::shared_ptr<UserBoundMethod>>(v))      \
        {                                                            \
            push("<bound method>");                                  \
        }                                                            \
        else if (value_is<std::shared_ptr<BoundMethod>>(v))          \
        {                                                            \
            push("<native method>");                                 \
        }                                                            \
        else if (value_is<std::shared_ptr<Dict>>(v))                 \
        {                                                            \
            push("<dict>");                                          \
        }                                                            \
        else                                                         \
            push("<unknown>");                                       \
                                                                     \
        break;                                                       \
    }

#define NATIVE_GLOBALS_INSTANCE_METHODS                                                  \
    case NativeMethod::INSTANCE_METHODS:                                                 \
    {                                                                                    \
        if (args.size() != 1)                                                            \
            throw std::runtime_error("methods() expects 1 argument");                    \
                                                                                         \
        if (!value_is<std::shared_ptr<Instance>>(args[0]))                               \
            throw std::runtime_error("methods() argument must be an Instance");          \
                                                                                         \
        auto instance = value_get<std::shared_ptr<Instance>>(args[0]);                   \
        if (!instance)                                                                   \
            throw std::runtime_error("The instance is null");                            \
        if (!instance->klass)                                                            \
            throw std::runtime_error("The instance has no class");                       \
                                                                                         \
        auto methods_array = std::make_shared<Array>();                                  \
        std::unordered_set<std::string> seen;                                            \
                                                                                         \
        for (auto klass = instance->klass; klass != nullptr; klass = klass->super_class) \
        {                                                                                \
            for (const auto &[name, func] : klass->methods)                              \
            {                                                                            \
                if (!name.empty() && seen.insert(name).second)                           \
                    methods_array->elements.push_back(name);                             \
            }                                                                            \
        }                                                                                \
                                                                                         \
        for (const auto &[field_name, value] : instance->fields)                         \
        {                                                                                \
            if (value_is<std::shared_ptr<UserBoundMethod>>(value) ||                     \
                value_is<std::shared_ptr<BoundMethod>>(value))                           \
            {                                                                            \
                if (!field_name.empty() && seen.insert(field_name).second)               \
                    methods_array->elements.push_back(field_name);                       \
            }                                                                            \
        }                                                                                \
                                                                                         \
        push(methods_array);                                                             \
        break;                                                                           \
    }
