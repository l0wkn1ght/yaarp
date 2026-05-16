#include "yaarp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t stop = 0;

void handle_sig(int s) { (void)s; stop=1; }

int main(int argc, char **argv) {
    signal(SIGINT, handle_sig);
    Config cfg={0}; cfg.mode=M_INTERACTIVE;
    
    // Arg parsing
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"-S")) cfg.mode=M_INSTALL;
        else if(!strcmp(argv[i],"-R")) cfg.mode=M_REMOVE;
        else if(!strcmp(argv[i],"-Ss")) {cfg.mode=M_SEARCH;if(i+1<argc)strcpy(cfg.query,argv[++i]);}
        else if(!strcmp(argv[i],"-Sy")) cfg.mode=M_SYNC;
        else if(!strcmp(argv[i],"-Su")) cfg.mode=M_UPGRADE;
        else if(!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h")) {
            printf("yaarp " VER " — Arch Linux package helper\n"
                   "Usage: yaarp [-S|-R|-Ss <query>|-Sy|-Su] [--help]\n"
                   "  (no args)  interactive fzf picker\n"
                   "  -S         install selected packages\n"
                   "  -R         remove packages\n"
                   "  -Ss Q      fuzzy-search and print results\n"
                   "  -Sy        refresh sync databases\n"
                   "  -Su        upgrade all packages\n"
                   "  --help     this help\n");
            return 0;
        }
        else if(!strcmp(argv[i],"--preview-info")) {
            // Internal preview mode
            PkgList *l=db_load_local(); Pkg *p=db_find(l, argv[i+1]);
            if(p){char*t=ui_preview_gen(p);puts(t);free(t);}
            db_free(l); return 0;
        }
    }

    PkgList *db = (cfg.local_only) ? db_load_local() : db_load_sync();
    cache_build(db);
    
    if(cfg.mode == M_INTERACTIVE || cfg.mode == M_INSTALL) {
        char **sel=NULL; int cnt=0;
        if(system("which fzf >/dev/null 2>&1")==0) ui_fzf_launch(db,&sel,&cnt);
        else ui_tui_launch(db,&sel,&cnt);
        
        if(cnt>0) pac_install(sel,cnt,false);
        for(int i=0;i<cnt;i++) free(sel[i]);
        free(sel);
    } else if(cfg.mode == M_SEARCH) {
        MatchList *r = search_fuzzy(db, cfg.query, 50);
        for(size_t i=0;i<r->count;i++) printf("%s/%s\n", r->items[i].pkg->repo, r->items[i].pkg->name);
        search_free(r);
    } else if(cfg.mode == M_SYNC) {
        char*a[]={"-Sy",NULL}; pac_exec(a,1);
    }
    
    db_free(db);
    return 0;
}