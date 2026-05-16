#include "../yaarp.h"
#include <ctype.h>
#include <string.h>

int fuzzy_match(const char *haystack, const char *needle) {
    if(!needle[0]) return 1000;
    int score=0;
    const char *h=haystack, *n=needle;
    while(*h) {
        if(tolower(*h)==tolower(*n)) {
            score += 10; // Match bonus
            if(h==haystack || !isalnum(*(h-1))) score += 5; // Word boundary
            n++;
        }
        h++;
    }
    return (*n ? -1 : score);
}

MatchList *search_fuzzy(PkgList *db, const char *q, int limit) {
    MatchList *ml = xmalloc(sizeof(MatchList));
    ml->cap=db->count; ml->count=0;
    ml->items = xcalloc(ml->cap, sizeof(Match));
    
    for(size_t i=0; i<db->count; i++) {
        int s = fuzzy_match(db->items[i].name, q);
        if(s > 0) {
            ml->items[ml->count].pkg=&db->items[i]; ml->items[ml->count++].score=s;
        }
    }
    // Bubble sort by score desc
    for(size_t i=0;i<ml->count;i++)
        for(size_t j=i+1;j<ml->count;j++)
            if(ml->items[j].score>ml->items[i].score) { Match t=ml->items[i];ml->items[i]=ml->items[j];ml->items[j]=t;}
            
    if(limit>0 && (int)ml->count>limit) ml->count=limit;
    return ml;
}