#include "core/core.hpp"

std::string to_string(TokenType type)
{
    switch (type)
    {
    case LEFT_PAREN: return "LEFT_PAREN";
    case RIGHT_PAREN: return "RIGHT_PAREN";
    case LEFT_BRACE: return "LEFT_BRACE";
    case RIGHT_BRACE: return "RIGHT_BRACE";
    case LEFT_BRACKET: return "LEFT_BRACKET";
    case RIGHT_BRACKET: return "RIGHT_BRACKET";
    case COMMA: return "COMMA";
    case DOT: return "DOT";
    case MINUS: return "MINUS";
    case PLUS: return "PLUS";
    case SEMICOLON: return "SEMICOLON";
    case SLASH: return "SLASH";
    case STAR: return "STAR";
    case COLON: return "COLON";
    case QUESTION: return "QUESTION";
    case BANG: return "BANG";
    case BANG_EQUAL: return "BANG_EQUAL";
    case EQUAL: return "EQUAL";
    case EQUAL_EQUAL: return "EQUAL_EQUAL";
    case GREATER: return "GREATER";
    case GREATER_EQUAL: return "GREATER_EQUAL";
    case LESS: return "LESS";
    case LESS_EQUAL: return "LESS_EQUAL";
    case PERCENT: return "PERCENT";
    case PLUS_EQUAL: return "PLUS_EQUAL";
    case MINUS_EQUAL: return "MINUS_EQUAL";
    case STAR_EQUAL: return "STAR_EQUAL";
    case SLASH_EQUAL: return "SLASH_EQUAL";
    case PERCENT_EQUAL: return "PERCENT_EQUAL";
    case STAR_STAR: return "STAR_STAR";
    case STAR_STAR_EQUAL: return "STAR_STAR_EQUAL";
    case PLUS_PLUS: return "PLUS_PLUS";
    case MINUS_MINUS: return "MINUS_MINUS";
    case IDENTIFIER: return "IDENTIFIER";
    case STRING: return "STRING";
    case NUMBER: return "NUMBER";
    case AND: return "AND";
    case CLASS: return "CLASS";
    case ELSE: return "ELSE";
    case FALSE: return "FALSE";
    case FUNCTION: return "FUNCTION";
    case FOR: return "FOR";
    case IF: return "IF";
    case NULL: return "NULL";
    case OR: return "OR";
    case RETURN: return "RETURN";
    case SUPER: return "SUPER";
    case THIS: return "THIS";
    case TRUE: return "TRUE";
    case VAR: return "VAR";
    case WHILE: return "WHILE";
    case CONST: return "CONST";
    case PUBLIC: return "PUBLIC";
    case PRIVATE: return "PRIVATE";
    case END_OF_FILE: return "END_OF_FILE";
    }
    return "UNKNOWN";
}

Token::Token(TokenType type, std::string lexeme, Value literal, int line)
    : type(type), lexeme(std::move(lexeme)), literal(std::move(literal)), line(line) {}

std::string Token::stringify() const
{
    std::string literal_text;
    switch (this->type)
    {
    case IDENTIFIER:
        literal_text = this->lexeme;
        break;
    case STRING:
        literal_text = value_get<std::string>(this->literal);
        break;
    case NUMBER:
        literal_text = std::to_string(value_get<double>(this->literal));
        break;
    case TRUE:
        literal_text = "true";
        break;
    case FALSE:
        literal_text = "false";
        break;
    default:
        literal_text = "null";
    }
    return ::to_string(this->type) + " " + this->lexeme + " " + literal_text;
}
