#include "core/core.hpp"

void report(int line, std::string where, std::string message)
{
    std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
    error_trigger = true;
}

void error(int line, std::string message)
{
    report(line, "", message);
}

void error(const Token &token, std::string message)
{
    if (token.type == END_OF_FILE)
    {
        report(token.line, " at end", message);
    }
    else
    {
        report(token.line, " at \"" + token.lexeme + "\"", message);
    }
}

void runtime_error(const RuntimeError &error) // I believe this is broken, doesn't even print the line
{
    std::cerr << error.what() << "\n[line " << error.token.line << "]\n";
    runtime_error_trigger = true;
}
