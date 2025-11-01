// ==================== CODE GENERATOR ====================

typedef struct {
    FILE *output;
    int label_count;
    int stack_offset;
    char var_names[100][256];
    int var_offsets[100];
    char var_types[100][64];
    int var_count;
    int loop_start_labels[50];
    int loop_end_labels[50];
    int loop_depth;
    int string_count;
    int array_sizes[100];
} CodeGen;

void codegen_init(CodeGen *gen, FILE *output) {
    gen->output = output;
    gen->label_count = 0;
    gen->stack_offset = 0;
    gen->var_count = 0;
    gen->loop_depth = 0;
    gen->string_count = 0;
}

int codegen_new_label(CodeGen *gen) {
    return gen->label_count++;
}

void codegen_emit(CodeGen *gen, const char *instruction) {
    fprintf(gen->output, "    %s\n", instruction);
}

void codegen_emit_label(CodeGen *gen, const char *label) {
    fprintf(gen->output, "%s:\n", label);
}

int codegen_find_var(CodeGen *gen, const char *name) {
    for (int i = gen->var_count - 1; i >= 0; i--) {
        if (strcmp(gen->var_names[i], name) == 0) {
            return gen->var_offsets[i];
        }
    }
    return -1;
}

const char* codegen_find_var_type(CodeGen *gen, const char *name) {
    for (int i = gen->var_count - 1; i >= 0; i--) {
        if (strcmp(gen->var_names[i], name) == 0) {
            return gen->var_types[i];
        }
    }
    return NULL;
}

void codegen_add_var(CodeGen *gen, const char *name) {
    gen->stack_offset += 8;
    strcpy(gen->var_names[gen->var_count], name);
    gen->var_offsets[gen->var_count] = gen->stack_offset;
    gen->array_sizes[gen->var_count] = 1;
    strcpy(gen->var_types[gen->var_count], "int");
    gen->var_count++;
}

void codegen_add_var_typed(CodeGen *gen, const char *name, const char *type) {
    if (strcmp(type, "string") == 0) {
        gen->stack_offset += 256;
    } else {
        gen->stack_offset += 8;
    }
    strcpy(gen->var_names[gen->var_count], name);
    gen->var_offsets[gen->var_count] = gen->stack_offset;
    gen->array_sizes[gen->var_count] = 1;
    strcpy(gen->var_types[gen->var_count], type);
    gen->var_count++;
}

void codegen_add_array(CodeGen *gen, const char *name, int size) {
    gen->stack_offset += 8 * size;
    strcpy(gen->var_names[gen->var_count], name);
    gen->var_offsets[gen->var_count] = gen->stack_offset;
    gen->array_sizes[gen->var_count] = size;
    strcpy(gen->var_types[gen->var_count], "int");
    gen->var_count++;
}

void codegen_expression(CodeGen *gen, ASTNode *node);
void codegen_statement(CodeGen *gen, ASTNode *node);

void codegen_expression(CodeGen *gen, ASTNode *node) {
    char buffer[512];

    if (node->type == AST_NUMBER) {
        sprintf(buffer, "mov rax, %s", node->value);
        codegen_emit(gen, buffer);
        codegen_emit(gen, "push rax");
        return;
    }

    if (node->type == AST_STRING) {
        fprintf(gen->output, "section .data\n");
        fprintf(gen->output, ".str%d: db ", gen->string_count);

        // Escapar caracteres especiales para NASM
        const char *s = node->value;
        int first = 1;
        while (*s) {
            if (!first) fprintf(gen->output, ", ");
            first = 0;

            if (*s == '\n') {
                fprintf(gen->output, "10");
            } else if (*s == '\t') {
                fprintf(gen->output, "9");
            } else if (*s == '\r') {
                fprintf(gen->output, "13");
            } else if (*s == '"') {
                fprintf(gen->output, "34");
            } else if (*s == '\\') {
                fprintf(gen->output, "92");
            } else {
                fprintf(gen->output, "%d", (unsigned char)*s);
            }
            s++;
        }
        fprintf(gen->output, ", 0\n");
        fprintf(gen->output, "section .text\n");

        sprintf(buffer, "lea rax, [rel .str%d]", gen->string_count);
        gen->string_count++;
        codegen_emit(gen, buffer);
        codegen_emit(gen, "push rax");
        return;
    }

    if (node->type == AST_ARRAY_ACCESS) {
        int base_offset = codegen_find_var(gen, node->value);
        if (base_offset != -1) {
            codegen_expression(gen, node->left);
            codegen_emit(gen, "pop rax");

            codegen_emit(gen, "imul rax, 8");

            sprintf(buffer, "lea rbx, [rbp-%d]", base_offset);
            codegen_emit(gen, buffer);
            codegen_emit(gen, "add rbx, rax");

            codegen_emit(gen, "mov rax, [rbx]");
            codegen_emit(gen, "push rax");
        } else {
            printf("Error: Array '%s' not found\n", node->value);
        }
        return;
    }

    if (node->type == AST_IDENTIFIER) {
        int offset = codegen_find_var(gen, node->value);
        if (offset != -1) {
            const char *var_type = codegen_find_var_type(gen, node->value);
            if (var_type && strcmp(var_type, "string") == 0) {
                sprintf(buffer, "lea rax, [rbp-%d]", offset);
                codegen_emit(gen, buffer);
            } else {
                sprintf(buffer, "mov rax, [rbp-%d]", offset);
                codegen_emit(gen, buffer);
            }
            codegen_emit(gen, "push rax");
        } else {
            printf("Error: Variable '%s' not found\n", node->value);
        }
        return;
    }

    if (node->type == AST_UNARY_OP) {
        codegen_expression(gen, node->left);
        codegen_emit(gen, "pop rax");

        if (strcmp(node->value, "!") == 0) {
            codegen_emit(gen, "test rax, rax");
            codegen_emit(gen, "setz al");
            codegen_emit(gen, "movzx rax, al");
        } else if (strcmp(node->value, "-") == 0) {
            codegen_emit(gen, "neg rax");
        }

        codegen_emit(gen, "push rax");
        return;
    }

    if (node->type == AST_BINARY_OP) {
        codegen_expression(gen, node->right);
        codegen_expression(gen, node->left);

        codegen_emit(gen, "pop rax");
        codegen_emit(gen, "pop rbx");

        if (strcmp(node->value, "+") == 0) {
            codegen_emit(gen, "add rax, rbx");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "-") == 0) {
            codegen_emit(gen, "sub rax, rbx");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "*") == 0) {
            codegen_emit(gen, "imul rax, rbx");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "/") == 0) {
            codegen_emit(gen, "xor rdx, rdx");
            codegen_emit(gen, "idiv rbx");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "%") == 0) {
            codegen_emit(gen, "xor rdx, rdx");
            codegen_emit(gen, "idiv rbx");
            codegen_emit(gen, "push rdx");
        } else if (strcmp(node->value, "==") == 0) {
            codegen_emit(gen, "cmp rax, rbx");
            codegen_emit(gen, "sete al");
            codegen_emit(gen, "movzx rax, al");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "!=") == 0) {
            codegen_emit(gen, "cmp rax, rbx");
            codegen_emit(gen, "setne al");
            codegen_emit(gen, "movzx rax, al");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "<") == 0) {
            codegen_emit(gen, "cmp rax, rbx");
            codegen_emit(gen, "setl al");
            codegen_emit(gen, "movzx rax, al");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, ">") == 0) {
            codegen_emit(gen, "cmp rax, rbx");
            codegen_emit(gen, "setg al");
            codegen_emit(gen, "movzx rax, al");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "<=") == 0) {
            codegen_emit(gen, "cmp rax, rbx");
            codegen_emit(gen, "setle al");
            codegen_emit(gen, "movzx rax, al");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, ">=") == 0) {
            codegen_emit(gen, "cmp rax, rbx");
            codegen_emit(gen, "setge al");
            codegen_emit(gen, "movzx rax, al");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "&&") == 0) {
            codegen_emit(gen, "and rax, rbx");
            codegen_emit(gen, "push rax");
        } else if (strcmp(node->value, "||") == 0) {
            codegen_emit(gen, "or rax, rbx");
            codegen_emit(gen, "push rax");
        }
        return;
    }

    if (node->type == AST_CALL) {
        if (strcmp(node->value, "exit") == 0) {
            if (node->child_count > 0) {
                codegen_expression(gen, node->children[0]);
                codegen_emit(gen, "pop rdi");
            } else {
                codegen_emit(gen, "mov rdi, 0");
            }
            codegen_emit(gen, "mov rax, 60");
            codegen_emit(gen, "syscall");
            return;
        }

        if (strcmp(node->value, "print") == 0) {
            for (int i = 0; i < node->child_count; i++) {
                ASTNode *arg = node->children[i];
                codegen_expression(gen, arg);
                codegen_emit(gen, "pop rdi");

                int is_string = 0;

                if (arg->type == AST_STRING) {
                    is_string = 1;
                } else if (arg->type == AST_IDENTIFIER) {
                    const char *var_type = codegen_find_var_type(gen, arg->value);
                    if (var_type && strcmp(var_type, "string") == 0) {
                        is_string = 1;
                    }
                } else if (arg->type == AST_ARRAY_ACCESS) {
                    const char *var_type = codegen_find_var_type(gen, arg->value);
                    if (var_type && strcmp(var_type, "string") == 0) {
                        is_string = 1;
                    }
                }

                if (is_string) {
                    codegen_emit(gen, "call print_str_no_nl");
                } else {
                    codegen_emit(gen, "call print_no_nl");
                }
            }
            codegen_emit(gen, "push rax");
            return;
        }

        if (strcmp(node->value, "input") == 0) {
            if (node->child_count > 0) {
                ASTNode *prompt = node->children[0];
                codegen_expression(gen, prompt);
                codegen_emit(gen, "pop rdi");
                codegen_emit(gen, "call print_str_no_nl");
            }
            codegen_emit(gen, "call input");
            codegen_emit(gen, "push rax");
            return;
        }

        if (strcmp(node->value, "str_to_int") == 0) {
            if (node->child_count > 0) {
                codegen_expression(gen, node->children[0]);
                codegen_emit(gen, "pop rdi");
                codegen_emit(gen, "call str_to_int");
            }
            codegen_emit(gen, "push rax");
            return;
        }

        const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

        for (int i = 0; i < node->child_count && i < 6; i++) {
            codegen_expression(gen, node->children[i]);
            codegen_emit(gen, "pop rax");
            sprintf(buffer, "mov %s, rax", arg_regs[i]);
            codegen_emit(gen, buffer);
        }

        sprintf(buffer, "call %s", node->value);
        codegen_emit(gen, buffer);

        codegen_emit(gen, "push rax");
        return;
    }
}

void codegen_statement(CodeGen *gen, ASTNode *node) {
    char buffer[512];

    if (node->type == AST_ARRAY_DECL) {
        int size = atoi(node->right->value);
        codegen_add_array(gen, node->value, size);
        return;
    }

    if (node->type == AST_VAR_DECL) {
        const char *var_type = node->left->value;
        codegen_add_var_typed(gen, node->value, var_type);

        if (node->right != NULL) {
            if (strcmp(var_type, "string") == 0) {
                codegen_expression(gen, node->right);
                codegen_emit(gen, "pop rsi");
                sprintf(buffer, "lea rdi, [rbp-%d]", gen->var_offsets[gen->var_count - 1]);
                codegen_emit(gen, buffer);
                codegen_emit(gen, "call strcpy_internal");
            } else {
                codegen_expression(gen, node->right);
                codegen_emit(gen, "pop rax");
                sprintf(buffer, "mov [rbp-%d], rax", gen->var_offsets[gen->var_count - 1]);
                codegen_emit(gen, buffer);
            }
        } else if (strcmp(var_type, "string") == 0) {
            sprintf(buffer, "lea rdi, [rbp-%d]", gen->var_offsets[gen->var_count - 1]);
            codegen_emit(gen, buffer);
            codegen_emit(gen, "mov byte [rdi], 0");
        }
        return;
    }

    if (node->type == AST_INCREMENT) {
        int offset = codegen_find_var(gen, node->value);
        if (offset != -1) {
            sprintf(buffer, "inc qword [rbp-%d]", offset);
            codegen_emit(gen, buffer);
        }
        return;
    }

    if (node->type == AST_DECREMENT) {
        int offset = codegen_find_var(gen, node->value);
        if (offset != -1) {
            sprintf(buffer, "dec qword [rbp-%d]", offset);
            codegen_emit(gen, buffer);
        }
        return;
    }

    if (node->type == AST_CONTINUE) {
        if (gen->loop_depth > 0) {
            sprintf(buffer, "jmp .L%d", gen->loop_start_labels[gen->loop_depth - 1]);
            codegen_emit(gen, buffer);
        } else {
            printf("Error: continue outside of loop\n");
        }
        return;
    }

    if (node->type == AST_ASSIGNMENT) {
        if (node->left != NULL) {
            int base_offset = codegen_find_var(gen, node->value);
            if (base_offset != -1) {
                codegen_expression(gen, node->right);
                codegen_emit(gen, "pop rbx");

                codegen_expression(gen, node->left);
                codegen_emit(gen, "pop rax");

                codegen_emit(gen, "imul rax, 8");

                sprintf(buffer, "lea rcx, [rbp-%d]", base_offset);
                codegen_emit(gen, buffer);
                codegen_emit(gen, "add rcx, rax");

                codegen_emit(gen, "mov [rcx], rbx");
            }
            return;
        }

        int offset = codegen_find_var(gen, node->value);
        if (offset != -1) {
            codegen_expression(gen, node->right);
            codegen_emit(gen, "pop rax");
            sprintf(buffer, "mov [rbp-%d], rax", offset);
            codegen_emit(gen, buffer);
        } else {
            printf("Error: Variable '%s' not found\n", node->value);
        }
        return;
    }

    if (node->type == AST_RETURN) {
        if (node->left != NULL) {
            codegen_expression(gen, node->left);
            codegen_emit(gen, "pop rax");
        } else {
            codegen_emit(gen, "mov rax, 0");
        }
        codegen_emit(gen, "add rsp, 256");
        codegen_emit(gen, "pop rbp");
        codegen_emit(gen, "ret");
        return;
    }

    if (node->type == AST_IF) {
        int else_label = codegen_new_label(gen);
        int end_label = codegen_new_label(gen);

        codegen_expression(gen, node->left);
        codegen_emit(gen, "pop rax");
        codegen_emit(gen, "cmp rax, 0");
        sprintf(buffer, "je .L%d", else_label);
        codegen_emit(gen, buffer);

        ASTNode *then_block = node->children[0];
        for (int i = 0; i < then_block->child_count; i++) {
            codegen_statement(gen, then_block->children[i]);
        }

        sprintf(buffer, "jmp .L%d", end_label);
        codegen_emit(gen, buffer);

        sprintf(buffer, ".L%d", else_label);
        codegen_emit_label(gen, buffer);

        if (node->child_count > 1) {
            ASTNode *else_block = node->children[1];
            for (int i = 0; i < else_block->child_count; i++) {
                codegen_statement(gen, else_block->children[i]);
            }
        }

        sprintf(buffer, ".L%d", end_label);
        codegen_emit_label(gen, buffer);
        return;
    }

    if (node->type == AST_LOOP) {
        int start_label = codegen_new_label(gen);
        int end_label = codegen_new_label(gen);

        gen->loop_start_labels[gen->loop_depth] = start_label;
        gen->loop_end_labels[gen->loop_depth] = end_label;
        gen->loop_depth++;

        sprintf(buffer, ".L%d", start_label);
        codegen_emit_label(gen, buffer);

        codegen_expression(gen, node->left);
        codegen_emit(gen, "pop rax");
        codegen_emit(gen, "cmp rax, 0");
        sprintf(buffer, "je .L%d", end_label);
        codegen_emit(gen, buffer);

        ASTNode *body = node->right;
        for (int i = 0; i < body->child_count; i++) {
            codegen_statement(gen, body->children[i]);
        }

        sprintf(buffer, "jmp .L%d", start_label);
        codegen_emit(gen, buffer);

        sprintf(buffer, ".L%d", end_label);
        codegen_emit_label(gen, buffer);

        gen->loop_depth--;
        return;
    }

    if (node->type == AST_BREAK) {
        if (gen->loop_depth > 0) {
            sprintf(buffer, "jmp .L%d", gen->loop_end_labels[gen->loop_depth - 1]);
            codegen_emit(gen, buffer);
        } else {
            printf("Error: break outside of loop\n");
        }
        return;
    }

    if (node->type == AST_CALL || node->type == AST_BINARY_OP) {
        codegen_expression(gen, node);
        codegen_emit(gen, "pop rax");
        return;
    }
}

void codegen_function(CodeGen *gen, ASTNode *node) {
    char buffer[512];

    sprintf(buffer, "%s", node->value);
    codegen_emit_label(gen, buffer);

    codegen_emit(gen, "push rbp");
    codegen_emit(gen, "mov rbp, rsp");

    int saved_stack_offset = gen->stack_offset;
    int saved_var_count = gen->var_count;
    gen->stack_offset = 0;
    gen->var_count = 0;

    ASTNode *params = node->children[0];
    for (int i = 0; i < params->child_count; i++) {
        ASTNode *param = params->children[i];
        codegen_add_var_typed(gen, param->value, param->left->value);
    }

    codegen_emit(gen, "sub rsp, 256");

    const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (int i = 0; i < params->child_count && i < 6; i++) {
        sprintf(buffer, "mov [rbp-%d], %s", gen->var_offsets[i], param_regs[i]);
        codegen_emit(gen, buffer);
    }

    ASTNode *body = node->children[1];
    for (int i = 0; i < body->child_count; i++) {
        codegen_statement(gen, body->children[i]);
    }

    codegen_emit(gen, "mov rax, 0");
    codegen_emit(gen, "add rsp, 256");
    codegen_emit(gen, "pop rbp");
    codegen_emit(gen, "ret");

    fprintf(gen->output, "\n");

    gen->stack_offset = saved_stack_offset;
    gen->var_count = saved_var_count;
}

void codegen_program(CodeGen *gen, ASTNode *node) {
    fprintf(gen->output, "section .data\n");
    fprintf(gen->output, "    digit_buffer db '0000000000', 10\n");
    fprintf(gen->output, "    digit_count dq 0\n");
    fprintf(gen->output, "    input_buffer times 256 db 0\n");
    fprintf(gen->output, "    newline db 10\n\n");

    fprintf(gen->output, "section .text\n");
    fprintf(gen->output, "global _start\n\n");

    fprintf(gen->output, "print_no_nl:\n");
    codegen_emit(gen, "push rbp");
    codegen_emit(gen, "mov rbp, rsp");
    codegen_emit(gen, "mov rax, rdi");
    codegen_emit(gen, "mov rcx, 10");
    codegen_emit(gen, "lea rsi, [rel digit_buffer]");
    codegen_emit(gen, "add rsi, 9");
    codegen_emit(gen, "mov qword [rel digit_count], 0");

    codegen_emit(gen, "test rax, rax");
    codegen_emit(gen, "jns .positive");
    codegen_emit(gen, "neg rax");
    codegen_emit(gen, "push rax");
    codegen_emit(gen, "mov rax, 1");
    codegen_emit(gen, "mov rdi, 1");
    codegen_emit(gen, "lea rsi, [rel .minus]");
    codegen_emit(gen, "mov rdx, 1");
    codegen_emit(gen, "syscall");
    codegen_emit(gen, "pop rax");

    fprintf(gen->output, ".positive:\n");
    codegen_emit(gen, "xor r8, r8");

    fprintf(gen->output, ".convert_loop:\n");
    codegen_emit(gen, "xor rdx, rdx");
    codegen_emit(gen, "div rcx");
    codegen_emit(gen, "add dl, '0'");
    codegen_emit(gen, "mov [rsi], dl");
    codegen_emit(gen, "dec rsi");
    codegen_emit(gen, "inc r8");
    codegen_emit(gen, "test rax, rax");
    codegen_emit(gen, "jnz .convert_loop");

    codegen_emit(gen, "inc rsi");
    codegen_emit(gen, "mov rax, 1");
    codegen_emit(gen, "mov rdi, 1");
    codegen_emit(gen, "mov rdx, r8");
    codegen_emit(gen, "syscall");

    codegen_emit(gen, "pop rbp");
    codegen_emit(gen, "ret");
    fprintf(gen->output, ".minus: db '-'\n\n");

    fprintf(gen->output, "print_str_no_nl:\n");
    codegen_emit(gen, "push rbp");
    codegen_emit(gen, "mov rbp, rsp");
    codegen_emit(gen, "mov rsi, rdi");
    codegen_emit(gen, "xor rdx, rdx");

    fprintf(gen->output, ".strlen:\n");
    codegen_emit(gen, "cmp byte [rsi + rdx], 0");
    codegen_emit(gen, "je .print");
    codegen_emit(gen, "inc rdx");
    codegen_emit(gen, "jmp .strlen");

    fprintf(gen->output, ".print:\n");
    codegen_emit(gen, "mov rax, 1");
    codegen_emit(gen, "mov rdi, 1");
    codegen_emit(gen, "syscall");

    codegen_emit(gen, "pop rbp");
    codegen_emit(gen, "ret\n");

    fprintf(gen->output, "input:\n");
    codegen_emit(gen, "push rbp");
    codegen_emit(gen, "mov rbp, rsp");
    codegen_emit(gen, "mov rax, 0");
    codegen_emit(gen, "mov rdi, 0");
    codegen_emit(gen, "lea rsi, [rel input_buffer]");
    codegen_emit(gen, "mov rdx, 255");
    codegen_emit(gen, "syscall");

    codegen_emit(gen, "dec rax");
    codegen_emit(gen, "lea rdi, [rel input_buffer]");
    codegen_emit(gen, "add rdi, rax");
    codegen_emit(gen, "mov byte [rdi], 0");

    codegen_emit(gen, "lea rax, [rel input_buffer]");
    codegen_emit(gen, "pop rbp");
    codegen_emit(gen, "ret\n");

    fprintf(gen->output, "str_to_int:\n");
    codegen_emit(gen, "push rbp");
    codegen_emit(gen, "mov rbp, rsp");
    codegen_emit(gen, "xor rax, rax");
    codegen_emit(gen, "xor rcx, rcx");
    codegen_emit(gen, "mov r8, 10");

    fprintf(gen->output, ".loop:\n");
    codegen_emit(gen, "movzx rdx, byte [rdi + rcx]");
    codegen_emit(gen, "cmp dl, 0");
    codegen_emit(gen, "je .done");
    codegen_emit(gen, "cmp dl, '0'");
    codegen_emit(gen, "jl .done");
    codegen_emit(gen, "cmp dl, '9'");
    codegen_emit(gen, "jg .done");
    codegen_emit(gen, "sub dl, '0'");
    codegen_emit(gen, "imul rax, r8");
    codegen_emit(gen, "add rax, rdx");
    codegen_emit(gen, "inc rcx");
    codegen_emit(gen, "jmp .loop");

    fprintf(gen->output, ".done:\n");
    codegen_emit(gen, "pop rbp");
    codegen_emit(gen, "ret\n");

    fprintf(gen->output, "strcpy_internal:\n");
    codegen_emit(gen, "push rbp");
    codegen_emit(gen, "mov rbp, rsp");
    codegen_emit(gen, "xor rcx, rcx");

    fprintf(gen->output, ".copy_loop:\n");
    codegen_emit(gen, "mov al, byte [rsi + rcx]");
    codegen_emit(gen, "mov byte [rdi + rcx], al");
    codegen_emit(gen, "test al, al");
    codegen_emit(gen, "je .copy_done");
    codegen_emit(gen, "inc rcx");
    codegen_emit(gen, "jmp .copy_loop");

    fprintf(gen->output, ".copy_done:\n");
    codegen_emit(gen, "pop rbp");
    codegen_emit(gen, "ret\n");

    int has_main = 0;
    for (int i = 0; i < node->child_count; i++) {
        if (node->children[i]->type == AST_FUNCTION) {
            if (strcmp(node->children[i]->value, "main") == 0) {
                has_main = 1;
            }
            codegen_function(gen, node->children[i]);
        }
    }

    if (!has_main) {
        printf("Error: No 'main' function found\n");
        exit(1);
    }

    fprintf(gen->output, "_start:\n");
    codegen_emit(gen, "call main");
    codegen_emit(gen, "mov rdi, rax");
    codegen_emit(gen, "mov rax, 60");
    codegen_emit(gen, "syscall");
}
