#include "../yaarp.h"
#include <string.h>

// Stub for now: delegates to linear scan or fuzzy
// In a full impl, this would build a hash map of 3-char sequences -> pkg list
MatchList *search_index_query(const char *q) {
    (void)q;
    return NULL; // Not implemented in v0.1, use fuzzy/regex
}