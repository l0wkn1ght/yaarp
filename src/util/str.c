#include "../yaarp.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

char *trim(char *s) {
    if(!s) return NULL;
    while(isspace(*s)) s++;
    if(*s==0) return s;
    char *e = s+strlen(s)-1;
    while(e>s && isspace(*e)) e--;
    e[1]=0; return s;
}

char **split(const char *s, char delim, int *count) {
    int cap=16, idx=0;
    char **arr = xmalloc(cap*sizeof(char*));
    const char *start = s;
    for(const char *p=s;;p++) {
        if(*p==delim || *p==0) {
            int len = p-start;
            arr[idx] = xmalloc(len+1);
            memcpy(arr[idx], start, len); arr[idx][len]=0;
            trim(arr[idx]); idx++;
            start = p+1;
            if(idx>=cap) arr=xrealloc(arr, (cap*=2)*sizeof(char*));
            if(*p==0) break;
        }
    }
    *count=idx; return arr;
}

bool starts_with(const char *s, const char *p) {
    return strncmp(s, p, strlen(p)) == 0;
}