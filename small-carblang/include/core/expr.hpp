struct ExpressionVisitor
{
    virtual Value visit_binary_expression(std::shared_ptr<Binary> expr) = 0;
    virtual Value visit_grouping_expression(std::shared_ptr<Grouping> expr) = 0;
    virtual Value visit_literal_expression(std::shared_ptr<Literal> expr) = 0;
    virtual Value visit_unary_expression(std::shared_ptr<Unary> expr) = 0;
    virtual Value visit_logical_expression(std::shared_ptr<Logical> expr) = 0;
    virtual Value visit_ternary_expression(std::shared_ptr<Ternary> expr) = 0;
    virtual Value visit_assign_expression(std::shared_ptr<Assign> expr) = 0;
    virtual Value visit_variable_expression(std::shared_ptr<Variable> expr) = 0;
    virtual Value visit_call_expression(std::shared_ptr<Call> expr) = 0;
    virtual Value visit_array_expression(std::shared_ptr<ArrayExpr> expr) = 0;
    virtual Value visit_index_expression(std::shared_ptr<IndexExpr> expr) = 0;
    virtual Value visit_index_assign_expression(std::shared_ptr<IndexAssign> expr) = 0;
    virtual Value visit_get_expression(std::shared_ptr<Get> expr) = 0;
    virtual Value visit_set_expression(std::shared_ptr<Set> expr) = 0;
    virtual Value visit_super_expression(std::shared_ptr<Super> expr) = 0;
    virtual Value visit_this_expression(std::shared_ptr<This> expr) = 0;
    virtual Value visit_dict_expression(std::shared_ptr<DictExpr> expr) = 0;
    virtual ~ExpressionVisitor() = default;
};

struct Expression
{
    virtual Value accept(ExpressionVisitor &visitor) = 0;
};

struct This : Expression, public std::enable_shared_from_this<This>
{
    Token keyword;

    This(Token keyword) : keyword(std::move(keyword)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_this_expression(shared_from_this());
    }
};

struct DictExpr : Expression, public std::enable_shared_from_this<DictExpr>
{
    std::vector<std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>>> entries;

    DictExpr(std::vector<std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>>> entries)
        : entries(std::move(entries)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_dict_expression(shared_from_this());
    }
};

struct Super : Expression, public std::enable_shared_from_this<Super>
{

    Token method;

    Super(Token method) : method(std::move(method)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_super_expression(shared_from_this());
    }
};

struct Set : Expression, public std::enable_shared_from_this<Set>
{
    std::shared_ptr<Expression> object;
    Token name;
    std::shared_ptr<Expression> value;

    Set(std::shared_ptr<Expression> object, Token name, std::shared_ptr<Expression> value)
        : object(std::move(object)), name(std::move(name)), value(std::move(value)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_set_expression(shared_from_this());
    }
};

struct ArrayExpr : Expression, public std::enable_shared_from_this<ArrayExpr>
{
    std::vector<std::shared_ptr<Expression>> elements;

    ArrayExpr(std::vector<std::shared_ptr<Expression>> elements)
        : elements(std::move(elements)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_array_expression(shared_from_this());
    }
};

struct IndexExpr : Expression, public std::enable_shared_from_this<IndexExpr>
{
    std::shared_ptr<Expression> array;
    std::shared_ptr<Expression> index;

    IndexExpr(std::shared_ptr<Expression> array,
              std::shared_ptr<Expression> index)
        : array(std::move(array)),
          index(std::move(index)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_index_expression(shared_from_this());
    }
};

struct IndexAssign : Expression, public std::enable_shared_from_this<IndexAssign>
{
    std::shared_ptr<Expression> array;
    std::shared_ptr<Expression> index;
    std::shared_ptr<Expression> value;

    IndexAssign(std::shared_ptr<Expression> array,
                std::shared_ptr<Expression> index,
                std::shared_ptr<Expression> value)
        : array(std::move(array)),
          index(std::move(index)),
          value(std::move(value)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_index_assign_expression(shared_from_this());
    }
};

struct Get : Expression, public std::enable_shared_from_this<Get>
{
    std::shared_ptr<Expression> object;
    Token name;

    Get(std::shared_ptr<Expression> object, Token name)
        : object(object), name(name) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_get_expression(shared_from_this());
    }
};

struct Call : Expression, public std::enable_shared_from_this<Call>
{
    std::shared_ptr<Expression> callee;
    Token paren;
    std::vector<std::shared_ptr<Expression>> arguments;

    Call(std::shared_ptr<Expression> callee,
         Token paren,
         std::vector<std::shared_ptr<Expression>> arguments)
        : callee(std::move(callee)),
          paren(std::move(paren)),
          arguments(std::move(arguments)) {}

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_call_expression(shared_from_this());
    }
};

struct Binary : Expression, public std::enable_shared_from_this<Binary>
{
    Binary(std::shared_ptr<Expression> left, Token operator_token, std::shared_ptr<Expression> right) : left{std::move(left)}, operator_token{std::move(operator_token)}, right{std::move(right)}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_binary_expression(shared_from_this());
    }

    const std::shared_ptr<Expression> left;
    const Token operator_token;
    const std::shared_ptr<Expression> right;
};

struct Grouping : Expression, public std::enable_shared_from_this<Grouping>
{
    Grouping(std::shared_ptr<Expression> expression) : expression{std::move(expression)}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_grouping_expression(shared_from_this());
    }

    const std::shared_ptr<Expression> expression;
};

struct Literal : Expression, public std::enable_shared_from_this<Literal>
{
    Literal(Value value) : value{std::move(value)}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_literal_expression(shared_from_this());
    }

    const Value value;
};

struct Logical : Expression, public std::enable_shared_from_this<Logical>
{
    Logical(std::shared_ptr<Expression> left, Token operator_token, std::shared_ptr<Expression> right) : left{std::move(left)}, operator_token{std::move(operator_token)}, right{std::move(right)}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_logical_expression(shared_from_this());
    }

    const std::shared_ptr<Expression> left;
    const Token operator_token;
    const std::shared_ptr<Expression> right;
};

// I got tired of regular if/else conditions so I made this, maybe switch/match will be added too in the future
struct Ternary : Expression, public std::enable_shared_from_this<Ternary>
{
    Ternary(std::shared_ptr<Expression> condition,
            std::shared_ptr<Expression> true_expression,
            std::shared_ptr<Expression> false_expression)
        : condition{std::move(condition)},
          true_expression{std::move(true_expression)},
          false_expression{std::move(false_expression)}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_ternary_expression(shared_from_this());
    }

    const std::shared_ptr<Expression> condition;
    const std::shared_ptr<Expression> true_expression;
    const std::shared_ptr<Expression> false_expression;
};

struct Unary : Expression, public std::enable_shared_from_this<Unary>
{
    Unary(Token operator_token, std::shared_ptr<Expression> right, bool postfix = false)
        : operator_token{std::move(operator_token)}, right{std::move(right)}, postfix{postfix}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_unary_expression(shared_from_this());
    }

    const Token operator_token;
    const std::shared_ptr<Expression> right;
    const bool postfix;
};

struct Assign : Expression, public std::enable_shared_from_this<Assign>
{
    Assign(Token name, std::shared_ptr<Expression> value) : name{std::move(name)}, value{std::move(value)}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_assign_expression(shared_from_this());
    }

    const Token name;
    const std::shared_ptr<Expression> value;
};

struct Variable : Expression, public std::enable_shared_from_this<Variable>
{
    Variable(Token name) : name{std::move(name)}
    {
    }

    Value accept(ExpressionVisitor &visitor) override
    {
        return visitor.visit_variable_expression(shared_from_this());
    }

    const Token name;
};
