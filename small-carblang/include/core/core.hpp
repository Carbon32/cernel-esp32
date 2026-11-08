#ifndef CORE_HPP
#define CORE_HPP

#include "runtime.hpp"

inline bool error_trigger = false;
inline bool runtime_error_trigger = false;

#include "expr.hpp"
#include "stmt.hpp"
#include "scanner.hpp"
#include "parser.hpp"
#include "compiler.hpp"

class Carblang
{
public:
    explicit Carblang(bool interactive_mode = false);
    std::optional<Chunk> compile_source(std::string code);
    void interactive();
    void run(std::string code);

private:
    VM vm;
    Compiler compiler;
    std::unordered_set<std::string> known_class_types;
};

#endif
