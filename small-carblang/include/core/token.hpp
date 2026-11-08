class Token
{
public:
    const TokenType type;
    const std::string lexeme;
    const Value literal;
    const int line;

    Token(TokenType type, std::string lexeme, Value literal, int line);
    std::string stringify() const;
};
