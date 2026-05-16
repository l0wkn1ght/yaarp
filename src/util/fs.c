#include "../yaarp.h"
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

// Helper to check if path exists and is dir
int is_dir(const char *path) {
    struct stat st; return stat(path, &st)==0 && S_ISDIR(st.st_mode);
}

// Read entire file into buffer (must free)
char *slurp(const char *path) {
    FILE *f = fopen(path, "r");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = xmalloc(len+1);
    fread(buf, 1, len, f);
    buf[len]=0; fclose(f);
    return buf;
}