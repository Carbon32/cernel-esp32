class RuntimeError : public std::runtime_error
{
public:
    const Token &token;
    RuntimeError(const Token &token, std::string message) : std::runtime_error{message.data()}, token{token}
    {
    }
};

void report(int line, std::string where, std::string message);
void error(int line, std::string message);
void error(const Token &token, std::string message);
void runtime_error(const RuntimeError &error);
