// This little system simply makes things easier, it always does. It makes management easier,
// adds extra compile-time type safety and offers a single point of control

template <typename T>
struct is_expression : std::false_type
{
};

template <typename T>
struct is_stmt : std::false_type
{
};

#define DECL_EXPR(NodeType)                         \
    template <>                                     \
    struct is_expression<NodeType> : std::true_type \
    {                                               \
    };
#define DECL_STMT(NodeType)                   \
    template <>                               \
    struct is_stmt<NodeType> : std::true_type \
    {                                         \
    };

DECL_EXPR(Literal)
DECL_EXPR(Variable)
DECL_EXPR(Binary)
DECL_EXPR(Unary)
DECL_EXPR(Grouping)
DECL_EXPR(Call)
DECL_EXPR(DictExpr)
DECL_EXPR(ArrayExpr)
DECL_EXPR(Assign)
DECL_EXPR(IndexAssign)
DECL_EXPR(Set)
DECL_EXPR(Logical)
DECL_EXPR(Ternary)
DECL_EXPR(This)
DECL_EXPR(Super)
DECL_EXPR(IndexExpr)
DECL_EXPR(Get)

DECL_STMT(ExprStmt)
DECL_STMT(PrintStmt)
DECL_STMT(PrintLnStmt)
DECL_STMT(VarStmt)
DECL_STMT(BlockStmt)
DECL_STMT(IfStmt)
DECL_STMT(WhileStmt)
DECL_STMT(ReturnStmt)
DECL_STMT(FunctionStmt)
DECL_STMT(ClassStmt)
DECL_STMT(PropertyStmt)

template <typename NodeType, typename... Args>
auto ANG(Args &&...args)
{
    if constexpr (is_expression<NodeType>::value)
    {
#ifdef DEBUG_AST
        std::cout << "Created node: " << typeid(NodeType).name() << std::endl;
#endif
        return std::make_shared<NodeType>(std::forward<Args>(args)...);
    }
    else if constexpr (is_stmt<NodeType>::value)
    {
#ifdef DEBUG_AST
        std::cout << "Created node: " << typeid(NodeType).name() << std::endl;
#endif
        return std::make_shared<NodeType>(std::forward<Args>(args)...);
    }
    else
    {
        static_assert(sizeof(NodeType) == 0, "NodeType must be Expression or Stmt");
    }
}
