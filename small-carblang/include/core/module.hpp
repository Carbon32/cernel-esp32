struct Module
{
    std::unordered_map<std::string, Value> methods;

    void set_method(const std::string &name, Value method)
    {
        methods[name] = std::move(method);
    }

    Value get_method(const std::string &name) const
    {
        auto it = methods.find(name);
        if (it == methods.end())
            throw std::runtime_error("Module has no method \"" + name + "\"");
        return it->second;
    }
};

std::shared_ptr<Instance> create_module_instance(Module &&module, const std::string &name);
std::shared_ptr<Instance> init_math();
std::shared_ptr<Instance> init_regex();
std::shared_ptr<Instance> init_time();
std::shared_ptr<Instance> init_encode();
std::shared_ptr<Instance> init_text();
std::shared_ptr<Instance> init_random();
std::shared_ptr<Instance> init_array();
std::shared_ptr<Instance> init_profiler();
