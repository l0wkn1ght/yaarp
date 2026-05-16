#include "../yaarp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>

int ui_fzf_launch(PkgList *db, char ***sel_out, int *cnt_out) {
    int pf[2], pt[2]; pipe(pf); pipe(pt);
    pid_t pid = fork();
    if(pid==0) {
        close(pf[1]); close(pt[0]);
        dup2(pf[0], 0); dup2(pt[1], 1);
        close(pf[0]); close(pt[1]);
        execlp("fzf","fzf","--preview","./yaarp --preview-info {1}","--ansi","--multi",(char*)NULL);
        exit(1);
    }
    close(pf[0]); close(pt[1]);
    
    FILE *in = fdopen(pf[1],"w");
    for(size_t i=0;i<db->count;i++)
        fprintf(in, "%s/%s %s %s\n", db->items[i].repo, db->items[i].name, db->items[i].version, db->items[i].desc);
    fclose(in);
    
    char **res=NULL; int c=0, cap=8; res=xcalloc(cap,sizeof(char*));
    FILE *out = fdopen(pt[0],"r");
    char line[MAX_LINE];
    while(fgets(line,sizeof(line),out)) {
        // extract name
        char *s = strchr(line,'/');
        if(s) {
            char *e = strchr(s+1,' ');
            if(e) {
                if(c>=cap) res=xrealloc(res,(cap*=2)*sizeof(char*));
                res[c]=xmalloc(e-s); memcpy(res[c],s+1,e-s-1); res[c][e-s-1]=0; c++;
            }
        }
    }
    fclose(out); waitpid(pid,NULL,0);
    *sel_out=res; *cnt_out=c;
    return 0;
}