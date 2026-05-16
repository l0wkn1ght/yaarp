#include "../yaarp.h"
#include <stdlib.h>
#include <string.h>

// In-memory index for fast lookups
static struct { char name[128]; Pkg *ptr; } *idx_map = NULL;
static size_t idx_count = 0;

void cache_build(PkgList *l) {
    if(idx_map) free(idx_map);
    idx_map = xcalloc(l->count, sizeof(*idx_map));
    idx_count = l->count;
    for(size_t i=0; i<l->count; i++) {
        strcpy(idx_map[i].name, l->items[i].name);
        idx_map[i].ptr = &l->items[i];
    }
}

Pkg *db_find(PkgList *l, const char *name) {
    // Linear scan if cache empty (small lists), hash lookup if built
    if(idx_count != l->count || !idx_map) {
        cache_build(l);
    }
    for(size_t i=0; i<idx_count; i++) {
        if(strcmp(idx_map[i].name, name)==0) return idx_map[i].ptr;
    }
    return NULL;
}

void db_free(PkgList *l) {
    if(!l) return;
    free(l->items); free(l);
    if(idx_map) { free(idx_map); idx_map=NULL; idx_count=0; }
}