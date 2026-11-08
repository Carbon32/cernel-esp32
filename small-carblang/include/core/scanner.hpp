class Scanner
{
public:
    Scanner(std::string src);
    std::vector<Token> scan_tokens();

private:
    static const std::map<std::string, TokenType> keywords;
    std::string src;
    std::vector<Token> tokens;
    int start = 0;
    int current = 0;
    int line = 1;

    bool match_alpha_string(const std::string &str);
    void scan_single_token();
    void identifier();
    void number();
    void string();
    bool match(char expected);
    char peek();
    char peek_next();
    bool is_alpha(char c);
    bool is_alpha_numeric(char c);
    bool is_digit(char c);
    bool at_end();
    char advance();
    void add_token(TokenType type);
    void add_token(TokenType type, Value literal);
};