struct StmtVisitor
{
    virtual Value visit_block_stmt(std::shared_ptr<BlockStmt> stmt) = 0;
    virtual Value visit_expression_stmt(std::shared_ptr<ExprStmt> stmt) = 0;
    virtual Value visit_if_stmt(std::shared_ptr<IfStmt> stmt) = 0;
    virtual Value visit_while_stmt(std::shared_ptr<WhileStmt> stmt) = 0;
    virtual Value visit_var_stmt(std::shared_ptr<VarStmt> stmt) = 0;
    virtual Value visit_function_stmt(std::shared_ptr<FunctionStmt> stmt) = 0;
    virtual Value visit_return_stmt(std::shared_ptr<ReturnStmt> stmt) = 0;
    virtual Value visit_class_stmt(std::shared_ptr<ClassStmt> stmt) = 0;
    virtual Value visit_property_stmt(std::shared_ptr<PropertyStmt> stmt) = 0;
    virtual ~StmtVisitor() = default;
};

struct Stmt
{
    virtual Value accept(StmtVisitor &visitor) = 0;
};

struct PropertyStmt : Stmt, public std::enable_shared_from_this<PropertyStmt>
{
    Token name;
    std::optional<std::string> type_name;
    std::shared_ptr<Expression> initializer;
    bool is_public = true;

    PropertyStmt(Token name, std::optional<std::string> type_name,
                 std::shared_ptr<Expression> initializer, bool is_public)
        : name(std::move(name)), type_name(std::move(type_name)),
          initializer(std::move(initializer)), is_public(is_public) {}

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_property_stmt(shared_from_this());
    }
};

struct ClassStmt : Stmt, public std::enable_shared_from_this<ClassStmt>
{
    Token name;
    std::shared_ptr<Variable> super_class;
    std::vector<std::shared_ptr<FunctionStmt>> methods;

    ClassStmt(Token name,
              std::shared_ptr<Variable> super_class,
              std::vector<std::shared_ptr<FunctionStmt>> methods)
        : name(std::move(name)),
          super_class(std::move(super_class)),
          methods(std::move(methods)) {}

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_class_stmt(shared_from_this());
    }
};

struct BlockStmt : Stmt, public std::enable_shared_from_this<BlockStmt>
{
    BlockStmt(std::vector<std::shared_ptr<Stmt>> statements) : statements{std::move(statements)}
    {
    }

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_block_stmt(shared_from_this());
    }

    const std::vector<std::shared_ptr<Stmt>> statements;
};

struct ExprStmt : Stmt, public std::enable_shared_from_this<ExprStmt>
{
    ExprStmt(std::shared_ptr<Expression> expression) : expression{std::move(expression)}
    {
    }

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_expression_stmt(shared_from_this());
    }
    const std::shared_ptr<Expression> expression;
};

struct IfStmt : Stmt, public std::enable_shared_from_this<IfStmt>
{
    IfStmt(std::shared_ptr<Expression> condition, std::shared_ptr<Stmt> then_branch, std::shared_ptr<Stmt> else_branch)
        : condition{std::move(condition)}, then_branch{std::move(then_branch)}, else_branch{std::move(else_branch)}
    {
    }

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_if_stmt(shared_from_this());
    }

    const std::shared_ptr<Expression> condition;
    const std::shared_ptr<Stmt> then_branch;
    const std::shared_ptr<Stmt> else_branch;
};

struct WhileStmt : Stmt, public std::enable_shared_from_this<WhileStmt>
{
    WhileStmt(std::shared_ptr<Expression> condition, std::shared_ptr<Stmt> body)
        : condition{std::move(condition)}, body{std::move(body)}
    {
    }

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_while_stmt(shared_from_this());
    }

    const std::shared_ptr<Expression> condition;
    const std::shared_ptr<Stmt> body;
};

struct VarStmt : Stmt, public std::enable_shared_from_this<VarStmt>
{
    VarStmt(
        Token name,
        std::shared_ptr<Expression> initializer,
        bool is_const = false,
        std::optional<std::string> type_name = std::nullopt)
        : name(std::move(name)),
          initializer(std::move(initializer)),
          is_const(is_const),
          type_name(std::move(type_name))
    {
    }

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_var_stmt(shared_from_this());
    }

    const Token name;
    const std::shared_ptr<Expression> initializer;

    bool is_const;

    std::optional<std::string> type_name;
};

struct Parameter
{
    Token name;
    std::optional<std::string> type_name;
};

struct FunctionStmt : Stmt, public std::enable_shared_from_this<FunctionStmt>
{
    Token name;
    std::vector<Parameter> params;
    std::vector<std::shared_ptr<Stmt>> body;
    std::optional<std::string> return_type;
    bool is_method = false;
    bool is_public = true;

    FunctionStmt(
        Token name,
        std::vector<Parameter> params,
        std::vector<std::shared_ptr<Stmt>> body,
        std::optional<std::string> return_type)
        : name(std::move(name)),
          params(std::move(params)),
          body(std::move(body)),
          return_type(std::move(return_type))
    {
    }

    Value accept(StmtVisitor &visitor) override
    {
        if (is_method)
            return {};
        return visitor.visit_function_stmt(shared_from_this());
    }
};

struct ReturnStmt : Stmt, public std::enable_shared_from_this<ReturnStmt>
{
    Token keyword;
    std::shared_ptr<Expression> value;

    ReturnStmt(Token keyword, std::shared_ptr<Expression> value)
        : keyword(std::move(keyword)), value(std::move(value)) {}

    Value accept(StmtVisitor &visitor) override
    {
        return visitor.visit_return_stmt(shared_from_this());
    }
};
