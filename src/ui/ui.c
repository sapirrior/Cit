#include "ui.h"
#include <stdio.h>
#include <stdarg.h>

void ui_init() {
    // Optional initialization
}

void ui_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(COLOR_FG_DEFAULT);
    vprintf(fmt, args);
    printf(COLOR_RESET "\n");
    va_end(args);
}

void ui_success(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(COLOR_FG_SUCCESS "✔ ");
    vprintf(fmt, args);
    printf(COLOR_RESET "\n");
    va_end(args);
}

void ui_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, COLOR_FG_DANGER "✖ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, COLOR_RESET "\n");
    va_end(args);
}

void ui_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(COLOR_FG_ATTENTION "! ");
    vprintf(fmt, args);
    printf(COLOR_RESET "\n");
    va_end(args);
}

void ui_header(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(COLOR_BOLD COLOR_FG_DEFAULT);
    vprintf(fmt, args);
    printf(COLOR_RESET "\n");
    va_end(args);
}
