// ==================== PARSER ====================

typedef struct {
    Lexer *lexer;
    Token current_token;
    Token peek_token;
} Parser;

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    parser->peek_token = lexer_next_token(lexer);
}

void parser_advance(Parser *parser) {
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(parser->lexer);
}

void parser_skip_newlines(Parser *parser) {
    while (parser->current_token.type == TOKEN_NEWLINE) {
        parser_advance(parser);
    }
}

int parser_expect(Parser *parser, TokenType type) {
    if (parser->current_token.type == type) {
        parser_advance(parser);
        return 1;
    }
    error("Expected token type %d but got %d at line %d\n",
          type, parser->current_token.type, parser->current_token.line);
    return 0;
}

ASTNode* parser_parse_expression(Parser *parser);
ASTNode* parser_parse_statement(Parser *parser);
ASTNode* parser_parse_primary(Parser *parser);

ASTNode* parser_parse_unary(Parser *parser) {
    if (parser->current_token.type == TOKEN_NOT ||
        parser->current_token.type == TOKEN_MINUS) {
        char op[8];
    strcpy(op, parser->current_token.value);
    parser_advance(parser);

    ASTNode *operand = parser_parse_unary(parser);
    ASTNode *node = ast_create_node(AST_UNARY_OP, op);
    node->left = operand;
    return node;
        }

        return parser_parse_primary(parser);
}

ASTNode* parser_parse_primary(Parser *parser) {
    ASTNode *node = NULL;

    if (parser->current_token.type == TOKEN_NUMBER) {
        node = ast_create_node(AST_NUMBER, parser->current_token.value);
        parser_advance(parser);
        return node;
    }

    if (parser->current_token.type == TOKEN_STRING_LITERAL) {
        node = ast_create_node(AST_STRING, parser->current_token.value);
        parser_advance(parser);
        return node;
    }

    if (parser->current_token.type == TOKEN_IDENTIFIER) {
        char name[256];
        strcpy(name, parser->current_token.value);
        parser_advance(parser);

        if (parser->current_token.type == TOKEN_LBRACKET) {
            parser_advance(parser);
            ASTNode *index = parser_parse_expression(parser);
            parser_expect(parser, TOKEN_RBRACKET);

            node = ast_create_node(AST_ARRAY_ACCESS, name);
            node->left = index;
            return node;
        }

        if (parser->current_token.type == TOKEN_LPAREN) {
            node = ast_create_node(AST_CALL, name);
            parser_advance(parser);

            while (parser->current_token.type != TOKEN_RPAREN) {
                ASTNode *arg = parser_parse_expression(parser);
                ast_add_child(node, arg);

                if (parser->current_token.type == TOKEN_COMMA) {
                    parser_advance(parser);
                }
            }

            parser_expect(parser, TOKEN_RPAREN);
            return node;
        }

        node = ast_create_node(AST_IDENTIFIER, name);
        return node;
    }

    if (parser->current_token.type == TOKEN_LPAREN) {
        parser_advance(parser);
        node = parser_parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return node;
    }

    error("Unexpected token in primary expression at line %d\n",
          parser->current_token.line);
    return NULL;
}

ASTNode* parser_parse_term(Parser *parser) {
    ASTNode *left = parser_parse_unary(parser);

    while (parser->current_token.type == TOKEN_MULTIPLY ||
        parser->current_token.type == TOKEN_DIVIDE ||
        parser->current_token.type == TOKEN_MODULO) {
        char op[8];
    strcpy(op, parser->current_token.value);
    parser_advance(parser);

    ASTNode *right = parser_parse_unary(parser);
    ASTNode *node = ast_create_node(AST_BINARY_OP, op);
    node->left = left;
    node->right = right;
    left = node;
        }

        return left;
}

ASTNode* parser_parse_arithmetic(Parser *parser) {
    ASTNode *left = parser_parse_term(parser);

    while (parser->current_token.type == TOKEN_PLUS ||
        parser->current_token.type == TOKEN_MINUS) {
        char op[8];
    strcpy(op, parser->current_token.value);
    parser_advance(parser);

    ASTNode *right = parser_parse_term(parser);
    ASTNode *node = ast_create_node(AST_BINARY_OP, op);
    node->left = left;
    node->right = right;
    left = node;
        }

        return left;
}

ASTNode* parser_parse_comparison(Parser *parser) {
    ASTNode *left = parser_parse_arithmetic(parser);

    while (parser->current_token.type == TOKEN_EQUAL ||
        parser->current_token.type == TOKEN_NOT_EQUAL ||
        parser->current_token.type == TOKEN_LESS ||
        parser->current_token.type == TOKEN_GREATER ||
        parser->current_token.type == TOKEN_LESS_EQUAL ||
        parser->current_token.type == TOKEN_GREATER_EQUAL) {
        char op[8];
    strcpy(op, parser->current_token.value);
    parser_advance(parser);

    ASTNode *right = parser_parse_arithmetic(parser);
    ASTNode *node = ast_create_node(AST_BINARY_OP, op);
    node->left = left;
    node->right = right;
    left = node;
        }

        return left;
}

ASTNode* parser_parse_expression(Parser *parser) {
    ASTNode *left = parser_parse_comparison(parser);

    while (parser->current_token.type == TOKEN_AND ||
        parser->current_token.type == TOKEN_OR) {
        char op[8];
    strcpy(op, parser->current_token.value);
    parser_advance(parser);

    ASTNode *right = parser_parse_comparison(parser);
    ASTNode *node = ast_create_node(AST_BINARY_OP, op);
    node->left = left;
    node->right = right;
    left = node;
        }

        return left;
}

ASTNode* parser_parse_var_decl(Parser *parser) {
    char type[64];
    strcpy(type, parser->current_token.value);
    parser_advance(parser);

    char name[256];
    strcpy(name, parser->current_token.value);
    parser_expect(parser, TOKEN_IDENTIFIER);

    ASTNode *node;

    if (parser->current_token.type == TOKEN_LBRACKET) {
        parser_advance(parser);

        if (parser->current_token.type != TOKEN_NUMBER) {
            printf("Error: Expected array size\n");
            return NULL;
        }

        char size[64];
        strcpy(size, parser->current_token.value);
        parser_advance(parser);

        parser_expect(parser, TOKEN_RBRACKET);

        node = ast_create_node(AST_ARRAY_DECL, name);
        node->left = ast_create_node(AST_IDENTIFIER, type);
        node->right = ast_create_node(AST_NUMBER, size);

        return node;
    }

    node = ast_create_node(AST_VAR_DECL, name);
    node->left = ast_create_node(AST_IDENTIFIER, type);

    if (parser->current_token.type == TOKEN_ASSIGN) {
        parser_advance(parser);
        node->right = parser_parse_expression(parser);
    }

    return node;
}

ASTNode* parser_parse_assignment(Parser *parser) {
    char name[256];
    strcpy(name, parser->current_token.value);
    parser_expect(parser, TOKEN_IDENTIFIER);

    ASTNode *node;

    if (parser->current_token.type == TOKEN_LBRACKET) {
        parser_advance(parser);
        ASTNode *index = parser_parse_expression(parser);
        parser_expect(parser, TOKEN_RBRACKET);

        parser_expect(parser, TOKEN_ASSIGN);

        node = ast_create_node(AST_ASSIGNMENT, name);
        node->left = index;
        node->right = parser_parse_expression(parser);

        return node;
    }

    parser_expect(parser, TOKEN_ASSIGN);

    node = ast_create_node(AST_ASSIGNMENT, name);
    node->right = parser_parse_expression(parser);

    return node;
}

ASTNode* parser_parse_return(Parser *parser) {
    parser_expect(parser, TOKEN_RETURN);

    ASTNode *node = ast_create_node(AST_RETURN, "return");

    if (parser->current_token.type != TOKEN_NEWLINE &&
        parser->current_token.type != TOKEN_RBRACE) {
        node->left = parser_parse_expression(parser);
        }

        return node;
}

ASTNode* parser_parse_if(Parser *parser) {
    parser_expect(parser, TOKEN_IF);

    ASTNode *node = ast_create_node(AST_IF, "if");
    node->left = parser_parse_expression(parser);

    parser_skip_newlines(parser);
    parser_expect(parser, TOKEN_LBRACE);
    parser_skip_newlines(parser);

    ASTNode *then_block = ast_create_node(AST_BLOCK, "then");
    while (parser->current_token.type != TOKEN_RBRACE) {
        parser_skip_newlines(parser);
        if (parser->current_token.type == TOKEN_RBRACE) break;
        ast_add_child(then_block, parser_parse_statement(parser));
        parser_skip_newlines(parser);
    }
    parser_expect(parser, TOKEN_RBRACE);

    ast_add_child(node, then_block);

    parser_skip_newlines(parser);
    if (parser->current_token.type == TOKEN_ELSE) {
        parser_advance(parser);
        parser_skip_newlines(parser);

        if (parser->current_token.type == TOKEN_IF) {
            ASTNode *else_if = parser_parse_if(parser);
            ASTNode *else_block = ast_create_node(AST_BLOCK, "else");
            ast_add_child(else_block, else_if);
            ast_add_child(node, else_block);
        } else {
            parser_expect(parser, TOKEN_LBRACE);
            parser_skip_newlines(parser);

            ASTNode *else_block = ast_create_node(AST_BLOCK, "else");
            while (parser->current_token.type != TOKEN_RBRACE) {
                parser_skip_newlines(parser);
                if (parser->current_token.type == TOKEN_RBRACE) break;
                ast_add_child(else_block, parser_parse_statement(parser));
                parser_skip_newlines(parser);
            }
            parser_expect(parser, TOKEN_RBRACE);

            ast_add_child(node, else_block);
        }
    }

    return node;
}

ASTNode* parser_parse_loop(Parser *parser) {
    parser_expect(parser, TOKEN_LOOP);

    ASTNode *node = ast_create_node(AST_LOOP, "loop");
    node->left = parser_parse_expression(parser);

    parser_skip_newlines(parser);
    parser_expect(parser, TOKEN_LBRACE);
    parser_skip_newlines(parser);

    ASTNode *body = ast_create_node(AST_BLOCK, "body");
    while (parser->current_token.type != TOKEN_RBRACE) {
        parser_skip_newlines(parser);
        if (parser->current_token.type == TOKEN_RBRACE) break;
        ast_add_child(body, parser_parse_statement(parser));
        parser_skip_newlines(parser);
    }
    parser_expect(parser, TOKEN_RBRACE);

    node->right = body;

    return node;
}

ASTNode* parser_parse_statement(Parser *parser) {
    parser_skip_newlines(parser);

    if (parser->current_token.type == TOKEN_INT ||
        parser->current_token.type == TOKEN_FLOAT ||
        parser->current_token.type == TOKEN_BOOL ||
        parser->current_token.type == TOKEN_STRING) {
        return parser_parse_var_decl(parser);
        }

        if (parser->current_token.type == TOKEN_RETURN) {
            return parser_parse_return(parser);
        }

        if (parser->current_token.type == TOKEN_IF) {
            return parser_parse_if(parser);
        }

        if (parser->current_token.type == TOKEN_LOOP) {
            return parser_parse_loop(parser);
        }

        if (parser->current_token.type == TOKEN_BREAK) {
            ASTNode *node = ast_create_node(AST_BREAK, "break");
            parser_advance(parser);
            return node;
        }

        if (parser->current_token.type == TOKEN_CONTINUE) {
            ASTNode *node = ast_create_node(AST_CONTINUE, "continue");
            parser_advance(parser);
            return node;
        }

        if (parser->current_token.type == TOKEN_IDENTIFIER) {
            if (parser->peek_token.type == TOKEN_INCREMENT) {
                char name[256];
                strcpy(name, parser->current_token.value);
                parser_advance(parser);
                parser_advance(parser);

                ASTNode *node = ast_create_node(AST_INCREMENT, name);
                return node;
            }

            if (parser->peek_token.type == TOKEN_DECREMENT) {
                char name[256];
                strcpy(name, parser->current_token.value);
                parser_advance(parser);
                parser_advance(parser);

                ASTNode *node = ast_create_node(AST_DECREMENT, name);
                return node;
            }

            if (parser->peek_token.type == TOKEN_ASSIGN ||
                parser->peek_token.type == TOKEN_LBRACKET) {
                return parser_parse_assignment(parser);
                } else {
                    return parser_parse_expression(parser);
                }
        }

        return parser_parse_expression(parser);
}

ASTNode* parser_parse_import(Parser *parser) {
    parser_expect(parser, TOKEN_IMPORT);

    if (parser->current_token.type != TOKEN_STRING_LITERAL) {
        error("Expected string literal after import\n");
        return NULL;
    }

    ASTNode *node = ast_create_node(AST_IMPORT, parser->current_token.value);
    parser_advance(parser);

    return node;
}

ASTNode* parser_parse_function(Parser *parser) {
    parser_expect(parser, TOKEN_FUNC);

    char name[256];
    strcpy(name, parser->current_token.value);
    parser_expect(parser, TOKEN_IDENTIFIER);

    ASTNode *node = ast_create_node(AST_FUNCTION, name);

    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *params = ast_create_node(AST_BLOCK, "params");
    while (parser->current_token.type != TOKEN_RPAREN) {
        if (parser->current_token.type == TOKEN_INT ||
            parser->current_token.type == TOKEN_FLOAT ||
            parser->current_token.type == TOKEN_BOOL ||
            parser->current_token.type == TOKEN_STRING) {
            ast_add_child(params, parser_parse_var_decl(parser));
            }

            if (parser->current_token.type == TOKEN_COMMA) {
                parser_advance(parser);
            }
    }
    parser_expect(parser, TOKEN_RPAREN);

    ast_add_child(node, params);

    parser_skip_newlines(parser);
    parser_expect(parser, TOKEN_LBRACE);
    parser_skip_newlines(parser);

    ASTNode *body = ast_create_node(AST_BLOCK, "body");
    while (parser->current_token.type != TOKEN_RBRACE) {
        parser_skip_newlines(parser);
        if (parser->current_token.type == TOKEN_RBRACE) break;
        ast_add_child(body, parser_parse_statement(parser));
        parser_skip_newlines(parser);
    }
    parser_expect(parser, TOKEN_RBRACE);

    ast_add_child(node, body);

    return node;
}

ASTNode* parser_parse_program(Parser *parser) {
    ASTNode *program = ast_create_node(AST_PROGRAM, "program");

    parser_skip_newlines(parser);

    while (parser->current_token.type != TOKEN_EOF) {
        if (parser->current_token.type == TOKEN_IMPORT) {
            ast_add_child(program, parser_parse_import(parser));
        }
        else if (parser->current_token.type == TOKEN_FUNC) {
            ast_add_child(program, parser_parse_function(parser));
        }
        parser_skip_newlines(parser);
    }

    return program;
}
