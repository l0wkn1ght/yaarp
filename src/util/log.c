#include "../yaarp.h"
#include <stdio.h>
#include <stdarg.h>

void log_info(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[36m[yaarp]\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void log_err(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[31m[yaarp error]\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}