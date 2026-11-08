class Compiler : public ExpressionVisitor, public StmtVisitor
{
    Chunk chunk;
    struct Local
    {
        std::string name;
        int depth;
        bool is_const = false;
        std::optional<std::string> type_name;
    };

    std::vector<Local> locals;
    std::unordered_map<std::string, std::string> global_types;
    std::unordered_map<std::string, std::shared_ptr<Function>> function_signatures;
    std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<Function>>> class_signatures;
    std::unordered_map<std::string, std::string> class_superclasses;
    std::unordered_map<std::string, std::unordered_map<std::string, PropertyInfo>> class_properties;
    std::unordered_set<std::string> known_class_types;

    std::optional<std::string> current_return_type;
    std::optional<std::string> current_method_name;
    std::optional<std::string> current_super_class_name;
    std::optional<std::string> current_class_name;
    int scope_depth = 0;
    bool in_function = false;
    bool in_method = false;
    bool current_method_is_public = true;
    bool in_subclass_method = false;
    bool discard_assignment_result = false;
    bool assignment_result_was_discarded = false;

public:
    Chunk compile(const std::vector<std::shared_ptr<Stmt>> &statements,
                  std::unordered_set<std::string> known_class_types = {});
    void begin_scope();
    void end_scope();
    int resolve_local(const std::string &name);

    Value visit_literal_expression(std::shared_ptr<Literal> expr) override;
    Value visit_grouping_expression(std::shared_ptr<Grouping> expr) override;
    Value visit_unary_expression(std::shared_ptr<Unary> expr) override;
    Value visit_binary_expression(std::shared_ptr<Binary> expr) override;
    Value visit_logical_expression(std::shared_ptr<Logical> expr) override;
    Value visit_ternary_expression(std::shared_ptr<Ternary> expr) override;
    Value visit_assign_expression(std::shared_ptr<Assign> expr) override;
    Value visit_variable_expression(std::shared_ptr<Variable> expr) override;
    Value visit_call_expression(std::shared_ptr<Call> expr) override;
    Value visit_array_expression(std::shared_ptr<ArrayExpr> expr) override;
    Value visit_index_expression(std::shared_ptr<IndexExpr> expr) override;
    Value visit_index_assign_expression(std::shared_ptr<IndexAssign> expr) override;
    Value visit_get_expression(std::shared_ptr<Get> expr) override;
    Value visit_set_expression(std::shared_ptr<Set> expr) override;
    Value visit_super_expression(std::shared_ptr<Super> expr) override;
    Value visit_this_expression(std::shared_ptr<This> expr) override;
    Value visit_dict_expression(std::shared_ptr<DictExpr> expr) override;

    Value visit_expression_stmt(std::shared_ptr<ExprStmt> stmt) override;
    Value visit_var_stmt(std::shared_ptr<VarStmt> stmt) override;
    Value visit_if_stmt(std::shared_ptr<IfStmt> stmt) override;
    Value visit_while_stmt(std::shared_ptr<WhileStmt> stmt) override;
    Value visit_block_stmt(std::shared_ptr<BlockStmt> stmt) override;
    Value visit_function_stmt(std::shared_ptr<FunctionStmt> stmt) override;
    Value visit_return_stmt(std::shared_ptr<ReturnStmt> stmt) override;
    Value visit_class_stmt(std::shared_ptr<ClassStmt> stmt) override;
    Value visit_property_stmt(std::shared_ptr<PropertyStmt> stmt) override;

    std::optional<std::string> infer_expression_type(std::shared_ptr<Expression> expr);
    std::optional<Value> try_fold_constant(std::shared_ptr<Expression> expr);

private:
    void emit(OpCode op);
    void emit_byte(size_t byte);
    void emit_index(uint16_t index);
    void emit_constant(Value value);
    std::shared_ptr<Function> compile_function(FunctionStmt &stmt, bool is_constructor, bool is_method, bool has_super_class = false, std::optional<std::string> super_class_name = std::nullopt, std::optional<std::string> class_name = std::nullopt);
    int emit_jump(OpCode op);
    void patch_jump(int offset);
    void emit_loop(int loop_start);
};
