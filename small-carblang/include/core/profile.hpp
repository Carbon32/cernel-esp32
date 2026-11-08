struct ProfilerEntry
{
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::duration<double, std::milli> total{0};
    int count{0};
    bool running{false};
};

class GlobalProfiler
{
public:
    std::unordered_map<std::string, ProfilerEntry> entries;

    void start(const std::string &name)
    {
        if (name.empty())
            throw std::runtime_error("Profiler name cannot be empty");

        auto &entry = entries[name];

        if (entry.running)
            throw std::runtime_error("Profiler block already started: " + name);

        entry.start = std::chrono::high_resolution_clock::now();
        entry.running = true;
    }

    void end(const std::string &name)
    {
        if (name.empty())
            throw std::runtime_error("Profiler name cannot be empty");

        auto it = entries.find(name);
        if (it == entries.end())
            throw std::runtime_error("Profiler block not found: " + name);

        auto &entry = it->second;

        if (!entry.running)
            throw std::runtime_error("Profiler block was not started: " + name);

        auto now = std::chrono::high_resolution_clock::now();
        entry.total += now - entry.start;
        entry.count++;
        entry.running = false;
    }

    std::string report()
    {
        if (entries.empty())
        {
            return "No profiling data collected";
        }

        std::stringstream ss;
        for (auto &[name, entry] : entries)
        {
            ss << name
               << ": "
               << entry.total.count()
               << " ms over "
               << entry.count
               << " calls\n";
        }

        return ss.str();
    }

    void reset()
    {
        entries.clear();
    }
};