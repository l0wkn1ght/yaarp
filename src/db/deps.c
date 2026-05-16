#include "../yaarp.h"
#include <string.h>

int deps_resolve(PkgList *db, const char *name, PkgList *out) {
    Pkg *p = db_find(db, name);
    if(!p) return -1;
    
    for(int i=0; i<p->dep_count; i++) {
        // Simple check to avoid loops (basic)
        bool already=false;
        for(size_t j=0; j<out->count; j++) {
            if(strcmp(out->items[j].name, p->depends[i])==0) { already=true; break; }
        }
        if(!already) {
            Pkg *dep = db_find(db, p->depends[i]);
            if(dep) {
                if(out->count >= out->cap) { out->cap*=2; out->items=xrealloc(out->items,out->cap*sizeof(Pkg)); }
                out->items[out->count++] = *dep;
                deps_resolve(db, dep->name, out); // Recurse
            }
        }
    }
    return 0;
}