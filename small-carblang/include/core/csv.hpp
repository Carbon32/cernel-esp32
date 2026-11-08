class CsvParser
{
public:
    CsvParser(const std::string &src) : src(src) {}

    std::vector<std::vector<std::string>> parse()
    {
        std::vector<std::vector<std::string>> table;

        skip_newlines();

        while (pos < src.size())
        {
            table.push_back(parse_row());
            skip_newlines();
        }

        return table;
    }

private:
    const std::string &src;
    size_t pos = 0;

    char peek() const
    {
        if (pos >= src.size())
            return '\0';
        return src[pos];
    }

    char advance()
    {
        return src[pos++];
    }

    void skip_newlines()
    {
        while (peek() == '\n' || peek() == '\r')
            advance();
    }

    std::vector<std::string> parse_row()
    {
        std::vector<std::string> row;

        while (true)
        {
            row.push_back(parse_field());

            if (peek() == ',')
            {
                advance();
                continue;
            }

            break;
        }

        return row;
    }

    std::string parse_field()
    {
        if (peek() == '"')
            return parse_quoted_field();
        else
            return parse_unquoted_field();
    }

    std::string parse_quoted_field()
    {
        advance();
        std::string result;

        while (true)
        {
            if (pos >= src.size())
                throw std::runtime_error("Unterminated quoted field");

            char c = advance();

            if (c == '"')
            {
                if (peek() == '"')
                {
                    advance();
                    result += '"';
                }
                else
                {
                    break;
                }
            }
            else
            {
                result += c;
            }
        }

        return result;
    }

    std::string parse_unquoted_field()
    {
        std::string result;

        while (true)
        {
            char c = peek();

            if (c == ',' || c == '\n' || c == '\r' || c == '\0')
                break;

            result += advance();
        }

        return result;
    }
};