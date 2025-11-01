// ==================== AST ====================

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_VAR_DECL,
    AST_ASSIGNMENT,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_CALL,
    AST_RETURN,
    AST_IF,
    AST_SWITCH,
    AST_CASE,
    AST_LOOP,
    AST_BREAK,
    AST_CONTINUE,
    AST_BLOCK,
    AST_STRING,
    AST_ARRAY_DECL,
    AST_ARRAY_ACCESS,
    AST_IMPORT,
    AST_INCREMENT,
    AST_DECREMENT
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char value[256];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode **children;
    int child_count;
    int capacity;
} ASTNode;

ASTNode* ast_create_node(ASTNodeType type, char *value) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    if (value) strcpy(node->value, value);
    else node->value[0] = '\0';
    node->left = NULL;
    node->right = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->capacity = 0;
    return node;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent->child_count >= parent->capacity) {
        parent->capacity = parent->capacity == 0 ? 4 : parent->capacity * 2;
        parent->children = (ASTNode**)realloc(parent->children,
                                              parent->capacity * sizeof(ASTNode*));
    }
    parent->children[parent->child_count++] = child;
}
