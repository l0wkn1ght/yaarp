#include "../yaarp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *xmalloc(size_t s) {
    void *p = malloc(s);
    if (!p && s) {
        fprintf(stderr, "OOM\n");
        exit(1);
    }
    return p;
}

void *xcalloc(size_t n, size_t s) {
    void *p = calloc(n ,s);
    if (!p && n && s) {
        fprintf(stderr, "OOM\n");
        exit(1);
    }
    return p;
}

void *xrealloc(void *p, size_t s) {
    p = realloc(p, s);
    if (!p && s) {
        fprintf(stderr, "OOM\n");
        exit(1);
    }
    return p;
}

char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t l = strlen(s)+1;
    char *d = xmalloc(l);
    return memcpy(d, s, l);
}