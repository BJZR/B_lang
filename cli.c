// ==================== COLORS & CLI ====================

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_WHITE   "\033[1;37m"

void error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("%s[ERROR]%s ", COLOR_RED, COLOR_RESET);
    vprintf(format, args);
    printf("\n");
    va_end(args);
    exit(1);
}

void warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("%s[WARNING]%s ", COLOR_YELLOW, COLOR_RESET);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void success(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("%s[SUCCESS]%s ", COLOR_GREEN, COLOR_RESET);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("%s[INFO]%s ", COLOR_CYAN, COLOR_RESET);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
