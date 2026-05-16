#include "../yaarp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char *ui_preview_gen(const Pkg *p) {
    if(!p) return NULL;
    char *buf = xmalloc(2048);
    char date[64]="N/A";
    if(p->date) {
        time_t t=p->date; struct tm *tm=localtime(&t); strftime(date,sizeof(date),"%Y-%m-%d",tm);
    }
    snprintf(buf, 2048,
        "\033[1;37m%s\033[m \033[33m%s\033[m [\033[36m%s\033[m]\n"
        "\n  %s\n"
        "\n  Installed: %s | Size: %.1fMB",
        p->name, p->version, p->repo, p->desc,
        p->installed?"Yes":"No", p->size/1048576.0
    );
    return buf;
}