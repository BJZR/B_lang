// ==================== TOKEN DEFINITIONS ====================

typedef enum {
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_BOOL,
    TOKEN_STRING,
    TOKEN_IMPORT,
    TOKEN_FUNC,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_LOOP,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING_LITERAL,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int line;
} Token;

// ==================== LEXER ====================

typedef struct {
    char *source;
    int pos;
    int line;
    char current;
} Lexer;

void lexer_init(Lexer *lex, char *source) {
    lex->source = source;
    lex->pos = 0;
    lex->line = 1;
    lex->current = source[0];
}

void lexer_advance(Lexer *lex) {
    lex->pos++;
    if (lex->current == '\n') lex->line++;
    lex->current = lex->source[lex->pos];
}

void lexer_skip_whitespace(Lexer *lex) {
    while (lex->current == ' ' || lex->current == '\t' || lex->current == '\r') {
        lexer_advance(lex);
    }
}

void lexer_skip_comment(Lexer *lex) {
    if (lex->current == '/' && lex->source[lex->pos + 1] == '/') {
        lexer_advance(lex);
        lexer_advance(lex);
        while (lex->current != '\n' && lex->current != '\0') {
            lexer_advance(lex);
        }
    }

    if (lex->current == '/' && lex->source[lex->pos + 1] == '*') {
        lexer_advance(lex);
        lexer_advance(lex);

        while (lex->current != '\0') {
            if (lex->current == '*' && lex->source[lex->pos + 1] == '/') {
                lexer_advance(lex);
                lexer_advance(lex);
                break;
            }
            lexer_advance(lex);
        }
    }
}

Token lexer_make_number(Lexer *lex) {
    Token token;
    token.line = lex->line;
    int i = 0;

    while (isdigit(lex->current) || lex->current == '.') {
        token.value[i++] = lex->current;
        lexer_advance(lex);
    }
    token.value[i] = '\0';

    token.type = strchr(token.value, '.') ? TOKEN_FLOAT : TOKEN_NUMBER;
    return token;
}

Token lexer_make_identifier(Lexer *lex) {
    Token token;
    token.line = lex->line;
    int i = 0;

    while (isalnum(lex->current) || lex->current == '_') {
        token.value[i++] = lex->current;
        lexer_advance(lex);
    }
    token.value[i] = '\0';

    if (strcmp(token.value, "int") == 0) token.type = TOKEN_INT;
    else if (strcmp(token.value, "float") == 0) token.type = TOKEN_FLOAT;
    else if (strcmp(token.value, "bool") == 0) token.type = TOKEN_BOOL;
    else if (strcmp(token.value, "string") == 0) token.type = TOKEN_STRING;
    else if (strcmp(token.value, "import") == 0) token.type = TOKEN_IMPORT;
    else if (strcmp(token.value, "func") == 0) token.type = TOKEN_FUNC;
    else if (strcmp(token.value, "return") == 0) token.type = TOKEN_RETURN;
    else if (strcmp(token.value, "if") == 0) token.type = TOKEN_IF;
    else if (strcmp(token.value, "else") == 0) token.type = TOKEN_ELSE;
    else if (strcmp(token.value, "loop") == 0) token.type = TOKEN_LOOP;
    else if (strcmp(token.value, "break") == 0) token.type = TOKEN_BREAK;
    else if (strcmp(token.value, "continue") == 0) token.type = TOKEN_CONTINUE;
    else token.type = TOKEN_IDENTIFIER;

    return token;
}

Token lexer_make_string(Lexer *lex) {
    Token token;
    token.type = TOKEN_STRING_LITERAL;
    token.line = lex->line;
    int i = 0;

    lexer_advance(lex);

    while (lex->current != '"' && lex->current != '\0') {
        if (lex->current == '\\' && lex->source[lex->pos + 1] == 'n') {
            token.value[i++] = '\n';
            lexer_advance(lex);
            lexer_advance(lex);
        } else if (lex->current == '\\' && lex->source[lex->pos + 1] == 't') {
            token.value[i++] = '\t';
            lexer_advance(lex);
            lexer_advance(lex);
        } else if (lex->current == '\\' && lex->source[lex->pos + 1] == '"') {
            token.value[i++] = '"';
            lexer_advance(lex);
            lexer_advance(lex);
        } else {
            token.value[i++] = lex->current;
            lexer_advance(lex);
        }
    }
    token.value[i] = '\0';

    if (lex->current == '"') lexer_advance(lex);

    return token;
}

Token lexer_next_token(Lexer *lex) {
    Token token;

    while (lex->current != '\0') {
        lexer_skip_whitespace(lex);
        lexer_skip_comment(lex);

        if (lex->current == '\0') break;

        token.line = lex->line;

        if (isdigit(lex->current)) {
            return lexer_make_number(lex);
        }

        if (isalpha(lex->current) || lex->current == '_') {
            return lexer_make_identifier(lex);
        }

        if (lex->current == '"') {
            return lexer_make_string(lex);
        }

        if (lex->current == '\n') {
            token.type = TOKEN_NEWLINE;
            strcpy(token.value, "\\n");
            lexer_advance(lex);
            return token;
        }

        switch (lex->current) {
            case '=':
                lexer_advance(lex);
                if (lex->current == '=') {
                    token.type = TOKEN_EQUAL;
                    strcpy(token.value, "==");
                    lexer_advance(lex);
                } else {
                    token.type = TOKEN_ASSIGN;
                    strcpy(token.value, "=");
                }
                return token;
            case '!':
                lexer_advance(lex);
                if (lex->current == '=') {
                    token.type = TOKEN_NOT_EQUAL;
                    strcpy(token.value, "!=");
                    lexer_advance(lex);
                } else {
                    token.type = TOKEN_NOT;
                    strcpy(token.value, "!");
                }
                return token;
            case '<':
                lexer_advance(lex);
                if (lex->current == '=') {
                    token.type = TOKEN_LESS_EQUAL;
                    strcpy(token.value, "<=");
                    lexer_advance(lex);
                } else {
                    token.type = TOKEN_LESS;
                    strcpy(token.value, "<");
                }
                return token;
            case '>':
                lexer_advance(lex);
                if (lex->current == '=') {
                    token.type = TOKEN_GREATER_EQUAL;
                    strcpy(token.value, ">=");
                    lexer_advance(lex);
                } else {
                    token.type = TOKEN_GREATER;
                    strcpy(token.value, ">");
                }
                return token;
            case '&':
                lexer_advance(lex);
                if (lex->current == '&') {
                    token.type = TOKEN_AND;
                    strcpy(token.value, "&&");
                    lexer_advance(lex);
                    return token;
                }
                break;
            case '|':
                lexer_advance(lex);
                if (lex->current == '|') {
                    token.type = TOKEN_OR;
                    strcpy(token.value, "||");
                    lexer_advance(lex);
                    return token;
                }
                break;
            case '+':
                lexer_advance(lex);
                if (lex->current == '+') {
                    token.type = TOKEN_INCREMENT;
                    strcpy(token.value, "++");
                    lexer_advance(lex);
                } else {
                    token.type = TOKEN_PLUS;
                    strcpy(token.value, "+");
                }
                return token;
            case '-':
                lexer_advance(lex);
                if (lex->current == '-') {
                    token.type = TOKEN_DECREMENT;
                    strcpy(token.value, "--");
                    lexer_advance(lex);
                } else {
                    token.type = TOKEN_MINUS;
                    strcpy(token.value, "-");
                }
                return token;
            case '*':
                token.type = TOKEN_MULTIPLY;
                strcpy(token.value, "*");
                lexer_advance(lex);
                return token;
            case '/':
                token.type = TOKEN_DIVIDE;
                strcpy(token.value, "/");
                lexer_advance(lex);
                return token;
            case '%':
                token.type = TOKEN_MODULO;
                strcpy(token.value, "%");
                lexer_advance(lex);
                return token;
            case '(':
                token.type = TOKEN_LPAREN;
                strcpy(token.value, "(");
                lexer_advance(lex);
                return token;
            case ')':
                token.type = TOKEN_RPAREN;
                strcpy(token.value, ")");
                lexer_advance(lex);
                return token;
            case '{':
                token.type = TOKEN_LBRACE;
                strcpy(token.value, "{");
                lexer_advance(lex);
                return token;
            case '}':
                token.type = TOKEN_RBRACE;
                strcpy(token.value, "}");
                lexer_advance(lex);
                return token;
            case '[':
                token.type = TOKEN_LBRACKET;
                strcpy(token.value, "[");
                lexer_advance(lex);
                return token;
            case ']':
                token.type = TOKEN_RBRACKET;
                strcpy(token.value, "]");
                lexer_advance(lex);
                return token;
            case ',':
                token.type = TOKEN_COMMA;
                strcpy(token.value, ",");
                lexer_advance(lex);
                return token;
        }

        lexer_advance(lex);
    }

    token.type = TOKEN_EOF;
    strcpy(token.value, "EOF");
    token.line = lex->line;
    return token;
}
