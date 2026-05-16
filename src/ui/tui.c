#include "../yaarp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ui_tui_launch(PkgList *db, char ***sel_out, int *cnt_out) {
    (void)sel_out; (void)cnt_out;
    printf("fzf not found. Enter search term: ");
    char q[256]; fgets(q,sizeof(q),stdin); trim(q);
    
    MatchList *res = search_fuzzy(db, q, 20);
    if(!res || res->count==0) { printf("No results.\n"); return 1; }
    
    for(size_t i=0;i<res->count;i++)
        printf("%zu) %s/%s [%s]\n", i+1, res->items[i].pkg->repo, res->items[i].pkg->name, res->items[i].pkg->version);
    
    printf("Select (comma sep): "); 
    char inp[256]; fgets(inp,sizeof(inp),stdin); trim(inp);
    
    // Parse selection... simplified for brevity
    // ...
    search_free(res);
    return 0;
}