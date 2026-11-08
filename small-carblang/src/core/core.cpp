#include "core/core.hpp"

#ifdef ESP_PLATFORM
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#endif

namespace
{
#ifdef ESP_PLATFORM
void write_console(const char *text, size_t length)
{
    uart_write_bytes(UART_NUM_0, text, length);
}

bool read_interactive_line(std::string &line)
{
    static bool skip_lf = false;
    line.clear();
    while (true)
    {
        uint8_t input_byte;
        if (uart_read_bytes(UART_NUM_0, &input_byte, 1, portMAX_DELAY) != 1)
            return !line.empty();
        const int ch = input_byte;

        if (skip_lf && ch == '\n')
        {
            skip_lf = false;
            continue;
        }
        skip_lf = false;

        if (ch == '\r' || ch == '\n')
        {
            if (ch == '\r')
                skip_lf = true;
            write_console("\r\n", 2);
            return true;
        }

        if ((ch == '\b' || ch == 127) && !line.empty())
        {
            size_t erase_at = line.size() - 1;
            while (erase_at > 0 &&
                   (static_cast<unsigned char>(line[erase_at]) & 0xc0) == 0x80)
                --erase_at;
            line.erase(erase_at);
            write_console("\b \b", 3);
        }
        else if (ch >= 32 && ch != 127)
        {
            line.push_back(static_cast<char>(ch));
            const char echo = static_cast<char>(ch);
            write_console(&echo, 1);
        }
    }
}
#else
bool read_interactive_line(std::string &line)
{
    return static_cast<bool>(std::getline(std::cin, line));
}
#endif

bool source_is_complete(const std::string &source)
{
    int parens = 0, brackets = 0, braces = 0;
    bool in_string = false, escaped = false, line_comment = false, block_comment = false;
    for (size_t i = 0; i < source.size(); ++i)
    {
        const char c = source[i];
        const char next = i + 1 < source.size() ? source[i + 1] : '\0';
        if (line_comment)
        {
            if (c == '\n') line_comment = false;
            continue;
        }
        if (block_comment)
        {
            if (c == '*' && next == '/') { block_comment = false; ++i; }
            continue;
        }
        if (in_string)
        {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '/' && next == '/') { line_comment = true; ++i; continue; }
        if (c == '/' && next == '*') { block_comment = true; ++i; continue; }
        if (c == '"') { in_string = true; continue; }
        if (c == '(') ++parens; else if (c == ')') --parens;
        else if (c == '[') ++brackets; else if (c == ']') --brackets;
        else if (c == '{') ++braces; else if (c == '}') --braces;
    }
    return !in_string && !block_comment && parens <= 0 && brackets <= 0 && braces <= 0;
}
}

Carblang::Carblang(bool interactive_mode)
{
    vm.init_globals(interactive_mode);
}

std::optional<Chunk> Carblang::compile_source(std::string code)
{
    error_trigger = false;
    runtime_error_trigger = false;
    Scanner scanner(std::move(code));
    std::vector<Token> tokens = scanner.scan_tokens();
    if (error_trigger)
        return std::nullopt;
    Parser parser(tokens, known_class_types);
    std::vector<std::shared_ptr<Stmt>> statements = parser.parse();
    if (error_trigger)
        return std::nullopt;

    known_class_types = parser.class_types();
    return compiler.compile(statements, known_class_types);
}

void Carblang::interactive()
{
    std::cout << "Carblang interactive mode (type exit to quit)" << std::endl;
    std::string line;
    std::string pending;
    while (true)
    {
        std::cout << (pending.empty() ? "> " : "... ") << std::flush;
        if (!read_interactive_line(line))
            break;
        if (pending.empty() && (line == "exit" || line == "quit"))
            break;
        if (pending.empty() && line.empty())
            continue;

        if (!pending.empty()) pending.push_back('\n');
        pending += line;
        if (!source_is_complete(pending))
            continue;

        error_trigger = false;
        runtime_error_trigger = false;
        try
        {
            run(std::move(pending));
        }
        catch (const std::exception &exception)
        {
            std::cerr << exception.what() << std::endl;
        }
        pending.clear();
    }
}

void Carblang::run(std::string code)
{
    auto chunk = compile_source(std::move(code));
    if (!chunk)
        return;
    vm.interpret(*chunk);
}

