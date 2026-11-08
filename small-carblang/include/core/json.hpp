class JsonParser
{
public:
    JsonParser(const std::string &src) : src(src)
    {
        if (this->src.size() >= 3 &&
            (unsigned char)this->src[0] == 0xEF &&
            (unsigned char)this->src[1] == 0xBB &&
            (unsigned char)this->src[2] == 0xBF)
        {
            pos = 3;
        }
    }

    Value parse()
    {
        skip_whitespace();
        Value v = parse_value();
        skip_whitespace();
        if (pos != src.size())
            throw std::runtime_error("Unexpected trailing JSON data");
        return v;
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

    void skip_whitespace()
    {
        while (std::isspace(static_cast<unsigned char>(peek())))
            advance();
    }

    Value parse_value()
    {
        skip_whitespace();

        // std::cout << "peek=" << (int)(unsigned char)peek() << "\n";
        if (peek() == '"')
            return parse_string();
        if (peek() == '{')
            return parse_object();
        if (peek() == '[')
            return parse_array();
        if (std::isdigit(peek()) || peek() == '-')
            return parse_number();

        if (src.compare(pos, 4, "null") == 0)
        {
            pos += 4;
            return nullptr;
        }

        if (src.compare(pos, 4, "true") == 0)
        {
            pos += 4;
            return true;
        }

        if (src.compare(pos, 5, "false") == 0)
        {
            pos += 5;
            return false;
        }

        throw std::runtime_error("Invalid JSON value");
    }

    Value parse_string()
    {
        advance();
        std::string result;

        while (true)
        {
            if (pos >= src.size())
                throw std::runtime_error("Unterminated JSON string");

            char c = advance();

            if (c == '"')
                break;

            if (c == '\\')
            {
                char next = advance();
                switch (next)
                {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    throw std::runtime_error("Invalid escape in JSON");
                }
            }
            else
            {
                result += c;
            }
        }

        return result;
    }

    Value parse_number()
    {
        size_t start = pos;

        if (peek() == '-')
            advance();
        while (std::isdigit(static_cast<unsigned char>(peek())))
            advance();

        if (peek() == '.')
        {
            advance();
            while (std::isdigit(static_cast<unsigned char>(peek())))
                advance();
        }

        return std::stod(src.substr(start, pos - start));
    }

    Value parse_array()
    {
        advance();

        auto arr = std::make_shared<Array>();
        skip_whitespace();

        if (peek() == ']')
        {
            advance();
            return arr;
        }

        while (true)
        {
            arr->elements.push_back(parse_value());
            skip_whitespace();

            if (peek() == ']')
            {
                advance();
                break;
            }

            if (peek() != ',')
                throw std::runtime_error("Expected \",\" in JSON array");

            advance();
        }

        return arr;
    }

    Value parse_object()
    {
        advance();

        auto dict = std::make_shared<Dict>();
        skip_whitespace();

        if (peek() == '}')
        {
            advance();
            return dict;
        }

        while (true)
        {
            skip_whitespace();

            if (peek() != '"')
                throw std::runtime_error("Expected string key in JSON object");

            std::string key = value_get<std::string>(parse_string());

            skip_whitespace();

            if (peek() != ':')
                throw std::runtime_error("Expected \":\" in JSON object");

            advance();

            dict->entries[key] = parse_value();

            skip_whitespace();

            if (peek() == '}')
            {
                advance();
                break;
            }

            if (peek() != ',')
                throw std::runtime_error("Expected \",\" in JSON object");

            advance();
        }

        return dict;
    }
};
