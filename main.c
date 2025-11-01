#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "cli.c"
#include "lexer.c"
#include "ast.c"
#include "parser.c"
#include "codegen.c"



// ==================== MAIN ====================

char* read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        error("Could not open file %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char*)malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);

    return content;
}

void process_imports(ASTNode *program, const char *base_path) {
    for (int i = 0; i < program->child_count; i++) {
        if (program->children[i]->type == AST_IMPORT) {
            char *import_file = program->children[i]->value;
            char full_path[512];
            sprintf(full_path, "%s", import_file);
            info("Importing: %s", import_file);
            char *source = read_file(full_path);
            if (source) {
                Lexer lexer;
                lexer_init(&lexer, source);

                Parser parser;
                parser_init(&parser, &lexer);
                ASTNode *imported_ast = parser_parse_program(&parser);

                for (int j = 0; j < imported_ast->child_count; j++) {
                    if (imported_ast->children[j]->type == AST_FUNCTION) {
                        ast_add_child(program, imported_ast->children[j]);
                    }
                }

                free(source);
            }
        }
    }
}

void help(){
    printf("%sB Compiler v1%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%sUsage:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  b <command> [options] <file.b>\n\n");
    printf("%sCommands:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %scompile%s     Compile to ASM and executable\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sasm%s         Compile to ASM only\n", COLOR_GREEN, COLOR_RESET);
    printf("  %srun%s         Compile and run immediately\n", COLOR_GREEN, COLOR_RESET);
    printf("  %shelp%s        Show this help message\n\n", COLOR_GREEN, COLOR_RESET);
    printf("%sExamples:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  b compile program.b\n");
    printf("  b asm program.b\n");
    printf("  b run program.b\n\n");
    printf("%sFeatures:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  - Types: int, float, bool, string\n");
    printf("  - Control: if/else, continue, loop\n");
    printf("  - Operators: +, -, *, /, %%, ++, --\n");
    printf("  - Comparisons: ==, !=, <, >, <=, >=\n");
    printf("  - Logic: &&, ||, !\n");
    printf("  - Arrays: int arr[10]\n");
    printf("  - Functions: func name(int x) { }\n");
    printf("  - Built-ins: print(), input(), str_to_int(), exit()\n");
    printf("  - Import: import \"file.b\"\n");
}


int main(int argc, char **argv) {
    if (argc < 2) {
        help();
        return 1;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        help();
        return 0;
    }

    if (argc < 2) {
        error("Missing source file");
    }

    const char *command = argv[1];
    char *source = read_file(argv[2]);
    if (!source) return 1;

    info("B Compiler - Compiling %s...\n", argv[2]);

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);
    ASTNode *ast = parser_parse_program(&parser);

    process_imports(ast, ".");

    FILE *output = fopen("output.asm", "w");
    if (!output) {
        error("Could not create output file\n");
        return 1;
    }
    info("Generating assembly code...");
    CodeGen codegen;
    codegen_init(&codegen, output);
    codegen_program(&codegen, ast);

    fclose(output);
    free(source);

    success("Assembly generated: output.asm");


    if (strcmp(command, "compile") == 0 || strcmp(command, "run") == 0) {
        info("Assembling with NASM...");
        int ret = system("nasm -f elf64 output.asm -o output.o");
        if (ret != 0) {
            error("NASM assembly failed");
        }
        success("Object file created: output.o");

        info("Linking...");
        ret = system("ld output.o -o program");
        if (ret != 0) {
            error("Linking failed");
        }
        success("Executable created: program");

        if (strcmp(command, "run") == 0) {
            info("Running program...");
            printf("\n%s--- Program Output ---%s\n", COLOR_MAGENTA, COLOR_RESET);
            ret = system("./program");
            printf("%s--- End of Output ---%s\n", COLOR_MAGENTA, COLOR_RESET);
            printf("\n%sExit code: %d%s\n", COLOR_CYAN, WEXITSTATUS(ret), COLOR_RESET);
        }
    } else if (strcmp(command, "asm") == 0) {
        success("Assembly code ready: output.asm");
        info("To assemble and run manually:");
        printf("  nasm -f elf64 output.asm -o output.o\n");
        printf("  ld output.o -o program\n");
        printf("  ./program\n");
    } else {
        error("Unknown command: %s", command);
    }

    return 0;
}
