bool compare_values(const Value &a, const Value &b, TokenType op);

class VM
{
    Chunk *chunk = nullptr;
    const uint8_t *ip = nullptr;
    std::vector<Value> stack;
    std::unordered_map<std::string, Value> globals;
    std::unordered_set<std::string> const_variables;

    // Honestly the codebase is such a mess that updating it after not working on it for a while is torture
    // Future me, I can confirm
    struct GlobalCacheEntry
    {
        Value *value;
        bool is_const;
    };

    std::unordered_map<const uint8_t *, GlobalCacheEntry> global_cache;

    struct MethodCacheEntry
    {
        std::shared_ptr<Class> klass;
        std::shared_ptr<Function> method;
    };

    std::unordered_map<const uint8_t *, MethodCacheEntry> method_cache;
    std::vector<Value> native_args;
    std::vector<CallFrame> frames;
    GlobalProfiler profiler;
    void write_output(const std::string &text);

public:
    void interpret(Chunk &new_chunk);
    void init_globals(bool interactive_mode = false);

private:
    void run();
    Value pop();
    Value add_one(const Value &value);
    Value pop_constant();
    std::shared_ptr<Function> pop_function();
    std::shared_ptr<Class> pop_class();
    std::shared_ptr<Instance> pop_instance();
    std::shared_ptr<Instance> current_instance();
    void push(Value value);
    uint16_t read_short();
    uint8_t read_byte();
    bool is_truthy(const Value &value);
    bool value_shallow_equal(const Value &x, const Value &y);
    void print_value(const Value &value);
    void print_stack();
    std::string stringify(const Value &value);
    std::string json_escape(const std::string &s);
    std::string json_number(double value);
    std::string json_stringify(const Value &value);
    std::string csv_escape(const std::string &s);
    std::string csv_value(const Value &value);
    std::string csv_stringify(const Value &value);
    void dump_state(const char *where);
    Value read_constant();
    std::string read_input(const std::string &prompt);
};
