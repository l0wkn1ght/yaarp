#include "../yaarp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

static Pkg parse_desc(const char *path) {
    Pkg p = {0}; p.installed=true;
    FILE *f = fopen(path, "r");
    if(!f) return p;
    
    char line[MAX_LINE]; char field[64]=""; bool arr=false; int di=0;
    while(fgets(line, sizeof(line), f)) {
        char *t = trim(line);
        if(t[0]=='%' && t[strlen(t)-1]=='%') {
            arr=false; 
            strncpy(field, t+1, strlen(t)-2); field[strlen(t)-2]=0;
            if(!strcmp(field,"DEPENDS")) { arr=true; di=0; }
            continue;
        }
        if(t[0]==0) continue;
        
        if(!strcmp(field,"NAME")) strncpy(p.name, t, 127);
        else if(!strcmp(field,"VERSION")) strncpy(p.version, t, 63);
        else if(!strcmp(field,"DESC")) strncpy(p.desc, t, 511);
        else if(!strcmp(field,"ARCH")) strncpy(p.arch, t, 31);
        else if(!strcmp(field,"SIZE") || !strcmp(field,"ISIZE")) p.size=strtoull(t,NULL,10);
        else if(!strcmp(field,"BUILDDATE")) p.date=strtol(t,NULL,10);
        else if(arr && !strcmp(field,"DEPENDS") && di<MAX_DEPS)
            strncpy(p.depends[di++], t, 127);
    }
    p.dep_count = di;
    fclose(f); return p;
}

PkgList *db_load_local(void) {
    PkgList *l = xmalloc(sizeof(PkgList)); l->cap=256; l->count=0;
    l->items = xcalloc(l->cap, sizeof(Pkg));
    
    DIR *d = opendir(DB_LOCAL);
    if(!d) return l;
    
    struct dirent *ent;
    while((ent=readdir(d))) {
        if(ent->d_name[0]=='.') continue;
        char p[MAX_PATH]; snprintf(p,sizeof(p),"%s/%s/desc",DB_LOCAL,ent->d_name);
        Pkg pkg = parse_desc(p);
        if(pkg.name[0]) {
            if(l->count>=l->cap) { l->cap*=2; l->items=xrealloc(l->items,l->cap*sizeof(Pkg)); }
            l->items[l->count++] = pkg;
        }
    }
    closedir(d); return l;
}