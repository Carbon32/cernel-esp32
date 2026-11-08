struct IgnoreReturnValue
{
    bool operator==(const IgnoreReturnValue &) const noexcept { return true; }
};

class Value
{
public:
    enum class Kind : uint8_t
    {
        Null,
        Number,
        Boolean,
        String,
        Function,
        Array,
        NativeMethod,
        Instance,
        Class,
        UserMethod,
        Dict,
        Ignore
    };

private:
    union Payload
    {
        std::nullptr_t null;
        double number;
        bool boolean;
        std::shared_ptr<std::string> string;
        std::shared_ptr<Function> function;
        std::shared_ptr<Array> array;
        std::shared_ptr<BoundMethod> native_method;
        std::shared_ptr<Instance> instance;
        std::shared_ptr<Class> klass;
        std::shared_ptr<UserBoundMethod> user_method;
        std::shared_ptr<Dict> dict;
        IgnoreReturnValue ignore;
        Payload() noexcept : null(nullptr) {}
        ~Payload() {}
    } payload;
    Kind kind_ = Kind::Null;

    void destroy() noexcept
    {
        switch (kind_)
        {
        case Kind::String:
            std::destroy_at(&payload.string);
            break;
        case Kind::Function:
            std::destroy_at(&payload.function);
            break;
        case Kind::Array:
            std::destroy_at(&payload.array);
            break;
        case Kind::NativeMethod:
            std::destroy_at(&payload.native_method);
            break;
        case Kind::Instance:
            std::destroy_at(&payload.instance);
            break;
        case Kind::Class:
            std::destroy_at(&payload.klass);
            break;
        case Kind::UserMethod:
            std::destroy_at(&payload.user_method);
            break;
        case Kind::Dict:
            std::destroy_at(&payload.dict);
            break;
        default:
            break;
        }
        payload.null = nullptr;
        kind_ = Kind::Null;
    }

    template <class T>
    void copy_payload(const Value &other)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            std::construct_at(&payload.string, other.payload.string);
            kind_ = Kind::String;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Function>>)
        {
            std::construct_at(&payload.function, other.payload.function);
            kind_ = Kind::Function;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Array>>)
        {
            std::construct_at(&payload.array, other.payload.array);
            kind_ = Kind::Array;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<BoundMethod>>)
        {
            std::construct_at(&payload.native_method, other.payload.native_method);
            kind_ = Kind::NativeMethod;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Instance>>)
        {
            std::construct_at(&payload.instance, other.payload.instance);
            kind_ = Kind::Instance;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Class>>)
        {
            std::construct_at(&payload.klass, other.payload.klass);
            kind_ = Kind::Class;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<UserBoundMethod>>)
        {
            std::construct_at(&payload.user_method, other.payload.user_method);
            kind_ = Kind::UserMethod;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Dict>>)
        {
            std::construct_at(&payload.dict, other.payload.dict);
            kind_ = Kind::Dict;
        }
    }

    void copy_from(const Value &other)
    {
        switch (other.kind_)
        {
        case Kind::Number:
            payload.number = other.payload.number;
            kind_ = Kind::Number;
            break;
        case Kind::Boolean:
            payload.boolean = other.payload.boolean;
            kind_ = Kind::Boolean;
            break;
        case Kind::String:
            copy_payload<std::string>(other);
            break;
        case Kind::Function:
            copy_payload<std::shared_ptr<Function>>(other);
            break;
        case Kind::Array:
            copy_payload<std::shared_ptr<Array>>(other);
            break;
        case Kind::NativeMethod:
            copy_payload<std::shared_ptr<BoundMethod>>(other);
            break;
        case Kind::Instance:
            copy_payload<std::shared_ptr<Instance>>(other);
            break;
        case Kind::Class:
            copy_payload<std::shared_ptr<Class>>(other);
            break;
        case Kind::UserMethod:
            copy_payload<std::shared_ptr<UserBoundMethod>>(other);
            break;
        case Kind::Dict:
            copy_payload<std::shared_ptr<Dict>>(other);
            break;
        case Kind::Ignore:
            payload.ignore = {};
            kind_ = Kind::Ignore;
            break;
        default:
            kind_ = Kind::Null;
            break;
        }
    }

    template <class T>
    void move_payload(Value &other) noexcept
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            std::construct_at(&payload.string, std::move(other.payload.string));
            kind_ = Kind::String;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Function>>)
        {
            std::construct_at(&payload.function, std::move(other.payload.function));
            kind_ = Kind::Function;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Array>>)
        {
            std::construct_at(&payload.array, std::move(other.payload.array));
            kind_ = Kind::Array;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<BoundMethod>>)
        {
            std::construct_at(&payload.native_method, std::move(other.payload.native_method));
            kind_ = Kind::NativeMethod;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Instance>>)
        {
            std::construct_at(&payload.instance, std::move(other.payload.instance));
            kind_ = Kind::Instance;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Class>>)
        {
            std::construct_at(&payload.klass, std::move(other.payload.klass));
            kind_ = Kind::Class;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<UserBoundMethod>>)
        {
            std::construct_at(&payload.user_method, std::move(other.payload.user_method));
            kind_ = Kind::UserMethod;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Dict>>)
        {
            std::construct_at(&payload.dict, std::move(other.payload.dict));
            kind_ = Kind::Dict;
        }
    }

    void move_from(Value &other) noexcept
    {
        switch (other.kind_)
        {
        case Kind::Number:
            payload.number = other.payload.number;
            kind_ = Kind::Number;
            break;
        case Kind::Boolean:
            payload.boolean = other.payload.boolean;
            kind_ = Kind::Boolean;
            break;
        case Kind::String:
            move_payload<std::string>(other);
            break;
        case Kind::Function:
            move_payload<std::shared_ptr<Function>>(other);
            break;
        case Kind::Array:
            move_payload<std::shared_ptr<Array>>(other);
            break;
        case Kind::NativeMethod:
            move_payload<std::shared_ptr<BoundMethod>>(other);
            break;
        case Kind::Instance:
            move_payload<std::shared_ptr<Instance>>(other);
            break;
        case Kind::Class:
            move_payload<std::shared_ptr<Class>>(other);
            break;
        case Kind::UserMethod:
            move_payload<std::shared_ptr<UserBoundMethod>>(other);
            break;
        case Kind::Dict:
            move_payload<std::shared_ptr<Dict>>(other);
            break;
        case Kind::Ignore:
            payload.ignore = {};
            kind_ = Kind::Ignore;
            break;
        default:
            kind_ = Kind::Null;
            break;
        }
        other.destroy();
    }

    template <class T>
    static constexpr Kind kind_for()
    {
        if constexpr (std::is_same_v<T, std::nullptr_t>)
            return Kind::Null;
        else if constexpr (std::is_same_v<T, double>)
            return Kind::Number;
        else if constexpr (std::is_same_v<T, bool>)
            return Kind::Boolean;
        else if constexpr (std::is_same_v<T, std::string>)
            return Kind::String;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Function>>)
            return Kind::Function;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Array>>)
            return Kind::Array;
        else if constexpr (std::is_same_v<T, std::shared_ptr<BoundMethod>>)
            return Kind::NativeMethod;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Instance>>)
            return Kind::Instance;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Class>>)
            return Kind::Class;
        else if constexpr (std::is_same_v<T, std::shared_ptr<UserBoundMethod>>)
            return Kind::UserMethod;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Dict>>)
            return Kind::Dict;
        else if constexpr (std::is_same_v<T, IgnoreReturnValue>)
            return Kind::Ignore;
        else
            static_assert(!sizeof(T), "Unsupported Value alternative");
    }

public:
    Value() noexcept = default;
    Value(std::nullptr_t) noexcept {}
    Value(double number) noexcept : kind_(Kind::Number) { payload.number = number; }
    Value(bool boolean) noexcept : kind_(Kind::Boolean) { payload.boolean = boolean; }
    Value(int number) noexcept : Value(static_cast<double>(number)) {}
    Value(size_t number) noexcept : Value(static_cast<double>(number)) {}
    Value(const char *text) : Value(std::string(text)) {}
    Value(const std::string &text) : kind_(Kind::String) { std::construct_at(&payload.string, std::make_shared<std::string>(text)); }
    Value(std::string &&text) : kind_(Kind::String) { std::construct_at(&payload.string, std::make_shared<std::string>(std::move(text))); }
    Value(IgnoreReturnValue) noexcept : kind_(Kind::Ignore) { payload.ignore = {}; }

    template <class T>
        requires requires { typename T::element_type; } &&
                 (std::is_same_v<T, std::shared_ptr<Function>> || std::is_same_v<T, std::shared_ptr<Array>> ||
                  std::is_same_v<T, std::shared_ptr<BoundMethod>> || std::is_same_v<T, std::shared_ptr<Instance>> ||
                  std::is_same_v<T, std::shared_ptr<Class>> || std::is_same_v<T, std::shared_ptr<UserBoundMethod>> ||
                  std::is_same_v<T, std::shared_ptr<Dict>>)
    Value(T value)
    {
        using E = typename T::element_type;
        if constexpr (std::is_same_v<E, Function>)
        {
            std::construct_at(&payload.function, std::move(value));
            kind_ = Kind::Function;
        }
        else if constexpr (std::is_same_v<E, Array>)
        {
            std::construct_at(&payload.array, std::move(value));
            kind_ = Kind::Array;
        }
        else if constexpr (std::is_same_v<E, BoundMethod>)
        {
            std::construct_at(&payload.native_method, std::move(value));
            kind_ = Kind::NativeMethod;
        }
        else if constexpr (std::is_same_v<E, Instance>)
        {
            std::construct_at(&payload.instance, std::move(value));
            kind_ = Kind::Instance;
        }
        else if constexpr (std::is_same_v<E, Class>)
        {
            std::construct_at(&payload.klass, std::move(value));
            kind_ = Kind::Class;
        }
        else if constexpr (std::is_same_v<E, UserBoundMethod>)
        {
            std::construct_at(&payload.user_method, std::move(value));
            kind_ = Kind::UserMethod;
        }
        else if constexpr (std::is_same_v<E, Dict>)
        {
            std::construct_at(&payload.dict, std::move(value));
            kind_ = Kind::Dict;
        }
    }

    Value(const Value &other) { copy_from(other); }
    Value(Value &&other) noexcept { move_from(other); }
    ~Value() { destroy(); }
    Value &operator=(const Value &other)
    {
        if (this != &other)
        {
            destroy();
            copy_from(other);
        }
        return *this;
    }
    Value &operator=(Value &&other) noexcept
    {
        if (this != &other)
        {
            destroy();
            move_from(other);
        }
        return *this;
    }

    Kind kind() const noexcept { return kind_; }
    template <class T>
    bool is() const noexcept { return kind_ == kind_for<T>(); }

    template <class T>
    T &get()
    {
        if (!is<T>())
            throw std::runtime_error("Invalid Value type access");
        if constexpr (std::is_same_v<T, std::nullptr_t>)
            return payload.null;
        else if constexpr (std::is_same_v<T, double>)
            return payload.number;
        else if constexpr (std::is_same_v<T, bool>)
            return payload.boolean;
        else if constexpr (std::is_same_v<T, std::string>)
            return *payload.string;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Function>>)
            return payload.function;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Array>>)
            return payload.array;
        else if constexpr (std::is_same_v<T, std::shared_ptr<BoundMethod>>)
            return payload.native_method;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Instance>>)
            return payload.instance;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Class>>)
            return payload.klass;
        else if constexpr (std::is_same_v<T, std::shared_ptr<UserBoundMethod>>)
            return payload.user_method;
        else if constexpr (std::is_same_v<T, std::shared_ptr<Dict>>)
            return payload.dict;
        else
            return payload.ignore;
    }
    template <class T>
    const T &get() const { return const_cast<Value *>(this)->get<T>(); }

    bool operator==(const Value &other) const
    {
        if (kind_ != other.kind_)
            return false;
        switch (kind_)
        {
        case Kind::Null:
            return true;
        case Kind::Number:
            return payload.number == other.payload.number;
        case Kind::Boolean:
            return payload.boolean == other.payload.boolean;
        case Kind::String:
            return *payload.string == *other.payload.string;
        case Kind::Function:
            return payload.function == other.payload.function;
        case Kind::Array:
            return payload.array == other.payload.array;
        case Kind::NativeMethod:
            return payload.native_method == other.payload.native_method;
        case Kind::Instance:
            return payload.instance == other.payload.instance;
        case Kind::Class:
            return payload.klass == other.payload.klass;
        case Kind::UserMethod:
            return payload.user_method == other.payload.user_method;
        case Kind::Dict:
            return payload.dict == other.payload.dict;
        case Kind::Ignore:
            return true;
        }
        return false;
    }
};

template <class T>
bool value_is(const Value &value) { return value.template is<T>(); }
template <class T>
T &value_get(Value &value) { return value.template get<T>(); }
template <class T>
const T &value_get(const Value &value) { return value.template get<T>(); }
template <class T>
T *value_get_if(Value *value) { return value && value->template is<T>() ? &value->template get<T>() : nullptr; }
template <class T>
const T *value_get_if(const Value *value) { return value && value->template is<T>() ? &value->template get<T>() : nullptr; }
static_assert(sizeof(Value) <= 24, "Value representation should stay compact");

struct Chunk
{
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::unordered_map<std::string, uint16_t> string_constant_indices;
    uint16_t add_constant(Value value)
    {
        if (const auto *text = value_get_if<std::string>(&value))
        {
            const auto existing = string_constant_indices.find(*text);
            if (existing != string_constant_indices.end())
                return existing->second;
        }

        if (constants.size() > std::numeric_limits<uint16_t>::max())
            throw std::runtime_error("A chunk cannot contain more than 65536 constants");

        const uint16_t index = static_cast<uint16_t>(constants.size());
        if (const auto *text = value_get_if<std::string>(&value))
            string_constant_indices.emplace(*text, index);
        constants.push_back(std::move(value));
        return index;
    }

    void write(uint8_t byte)
    {
        code.push_back(byte);
    }
};

struct CallFrame
{
    Chunk *chunk;
    const uint8_t *ip;
    size_t stack_start;
    bool is_constructor = false;
    std::shared_ptr<Function> function;
};

struct PropertyInfo
{
    std::string owner_class;
    bool is_public = true;
    std::optional<std::string> type_name;
};

struct Function
{
    std::string name;
    size_t arity;
    Chunk chunk;
    std::vector<std::optional<std::string>> param_types;
    std::optional<std::string> return_type;
    std::shared_ptr<Class> super_class;
    std::string owner_class;
    bool is_public = true;
    std::unordered_map<std::string, PropertyInfo> declared_properties;

    Function(std::string name,
             size_t arity,
             Chunk chunk,
             std::vector<std::optional<std::string>> param_types,
             std::optional<std::string> return_type = std::nullopt)
        : name(std::move(name)),
          arity(arity),
          chunk(std::move(chunk)),
          param_types(std::move(param_types)),
          return_type(std::move(return_type))
    {
    }
};

struct BoundMethod
{
    Value receiver;
    NativeMethod method;

    BoundMethod(Value receiver, NativeMethod method)
        : receiver(std::move(receiver)), method(method) {}
};

struct UserBoundMethod
{
    std::shared_ptr<Instance> receiver;
    std::shared_ptr<Function> method;

    UserBoundMethod(std::shared_ptr<Instance> receiver, std::shared_ptr<Function> method)
        : receiver(receiver), method(method) {}
};

struct Array
{
    std::vector<Value> elements;
};

struct Dict
{
    std::unordered_map<std::string, Value> entries;

    void set(const std::string &key, Value val)
    {
        entries[key] = std::move(val);
    }

    Value get(const std::string &key) const
    {
        auto it = entries.find(key);
        if (it != entries.end())
            return it->second;
        return nullptr;
    }

    bool contains(const std::string &key) const
    {
        return entries.find(key) != entries.end();
    }

    bool remove(const std::string &key)
    {
        return entries.erase(key) > 0;
    }

    size_t length() const
    {
        return entries.size();
    }

    void clear()
    {
        entries.clear();
    }

    std::shared_ptr<Dict> copy() const
    {
        auto d = std::make_shared<Dict>();
        d->entries = entries;
        return d;
    }
};

struct Instance
{
    std::shared_ptr<Class> klass;
    std::unordered_map<std::string, Value> fields;

    Instance(std::shared_ptr<Class> klass) : klass(std::move(klass)) {}
};

struct Class
{
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<Function>> methods;
    std::unordered_map<std::string, PropertyInfo> properties;
    std::shared_ptr<Class> super_class;
    bool is_user_defined = false;

    Class(std::string name, std::unordered_map<std::string, std::shared_ptr<Function>> methods,
          std::shared_ptr<Class> super_class = nullptr)
        : name(std::move(name)), methods(std::move(methods)), super_class(std::move(super_class)) {}

    std::shared_ptr<Function> find_method(const std::string &name) const
    {
        auto it = methods.find(name);
        if (it != methods.end())
            return it->second;
        if (super_class)
            return super_class->find_method(name);
        return nullptr;
    }

    const PropertyInfo *find_property(const std::string &name) const
    {
        auto it = properties.find(name);
        if (it != properties.end())
            return &it->second;
        return super_class ? super_class->find_property(name) : nullptr;
    }
};
