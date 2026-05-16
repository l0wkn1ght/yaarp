#include "../yaarp.h"
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

MatchList *search_regex(PkgList *db, const char *pat, int limit) {
    regex_t r; 
    if(regcomp(&r, pat, REG_ICASE|REG_NOSUB)!=0) return NULL;
    
    MatchList *ml = xmalloc(sizeof(MatchList));
    ml->cap=db->count; ml->count=0;
    ml->items = xcalloc(ml->cap, sizeof(Match));
    
    for(size_t i=0; i<db->count; i++) {
        if(regexec(&r, db->items[i].name, 0, NULL, 0)==0 ||
           regexec(&r, db->items[i].desc, 0, NULL, 0)==0) {
               ml->items[ml->count].pkg=&db->items[i]; ml->items[ml->count++].score=100;
        }
    }
    regfree(&r);
    if(limit>0 && (int)ml->count>limit) ml->count=limit;
    return ml;
}

void search_free(MatchList *m) { if(m){free(m->items);free(m);} }