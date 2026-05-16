#include "../yaarp.h"
#include <stdio.h>
#include <string.h>

PkgList *db_load_sync(void) {
    PkgList *l = xmalloc(sizeof(PkgList)); l->cap=1024; l->count=0;
    l->items = xcalloc(l->cap, sizeof(Pkg));
    
    // Strategy: Parse 'pacman -Sl' output. It's fast enough and always accurate.
    FILE *p = popen("pacman -Sl 2>/dev/null", "r");
    if(!p) return l;
    
    char line[MAX_LINE];
    while(fgets(line, sizeof(line), p)) {
        Pkg pkg = {0};
        // Format: REPO NAME VERSION SIZE [INST]
        char *tok = strtok(line, " ");
        if(tok) strncpy(pkg.repo, tok, 63);
        
        tok = strtok(NULL, " ");
        if(tok) strncpy(pkg.name, tok, 127);
        
        tok = strtok(NULL, " ");
        if(tok) strncpy(pkg.version, tok, 63);
        
        // Check installed flag (brackets)
        tok = strtok(NULL, " ");
        if(tok && strstr(tok, "[installed]")) pkg.installed = true;

        if(pkg.name[0]) {
             if(l->count>=l->cap) { l->cap*=2; l->items=xrealloc(l->items,l->cap*sizeof(Pkg)); }
             l->items[l->count++] = pkg;
        }
    }
    pclose(p); return l;
}