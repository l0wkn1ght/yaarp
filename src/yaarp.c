#include "yaarp.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

int pac_exec(char **args, int n) {
    pid_t pid=fork();
    if(pid==0) {
        char **a=xcalloc(n+2,sizeof(char*)); a[0]="pacman";
        for(int i=0;i<n;i++) { a[i+1]=args[i]; }
        a[n+1]=NULL;
        execvp("pacman",a); perror("exec"); exit(1);
    }
    int s; waitpid(pid,&s,0); return WEXITSTATUS(s);
}

int pac_install(char **pkgs, int n, bool asdep) {
    int i=0; char *a[n+4];
    a[i++]="-S"; a[i++]="--noconfirm"; if(asdep) a[i++]="--asdeps";
    for(int j=0;j<n;j++) a[i++]=pkgs[j];
    return pac_exec(a,i);
}