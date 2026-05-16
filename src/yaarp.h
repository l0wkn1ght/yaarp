#ifndef YAARP_H
#define YAARP_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <regex.h>

// Config
#define VER "0.1.0"
#define MAX_PATH 512
#define MAX_LINE 4096
#define MAX_DEPS 256
#define DB_LOCAL "/var/lib/pacman/local"
#define DB_SYNC  "/var/lib/pacman/sync"

// Types
typedef struct {
    char name[128], version[64], desc[512], repo[64], arch[32];
    char depends[MAX_DEPS][128]; int dep_count;
    size_t size; time_t date;
    bool installed;
} Pkg;

typedef struct { Pkg *items; size_t count, cap; } PkgList;
typedef struct { Pkg *pkg; int score; } Match;
typedef struct { Match *items; size_t count, cap; } MatchList;

typedef enum { M_INTERACTIVE, M_INSTALL, M_REMOVE, M_QUERY, M_SEARCH, M_SYNC, M_UPGRADE } Mode;
typedef struct {
    Mode mode;
    bool verbose, noconfirm, asdeps, cascade, local_only;
    char query[256];
} Config;

// Util (str/mem/fs/log)
void  *xmalloc(size_t s);
void  *xcalloc(size_t n, size_t s);
void  *xrealloc(void *p, size_t s);
char  *xstrdup(const char *s);
char  *trim(char *s);
char **split(const char *s, char d, int *c);
bool   starts_with(const char *s, const char *p);
void   log_info(const char *fmt, ...);
void   log_err(const char *fmt, ...);

// DB (local/sync/cache/deps)
PkgList *db_load_local(void);
PkgList *db_load_sync(void);
Pkg     *db_find(PkgList *l, const char *name); // Uses cache internally
void     cache_build(PkgList *l);
void     db_free(PkgList *l);
int      deps_resolve(PkgList *db, const char *name, PkgList *out);

// Search (fuzzy/regex/index)
MatchList *search_fuzzy(PkgList *db, const char *q, int limit);
MatchList *search_regex(PkgList *db, const char *pat, int limit);
void       search_index_build(PkgList *db);       // Build trigram index
MatchList *search_index_query(const char *q);      // Fast prefix lookup
void       search_free(MatchList *r);

// UI (fzf/preview/tui)
int  ui_fzf_launch(PkgList *db, char ***sel, int *count);
char *ui_preview_gen(const Pkg *p);
int  ui_tui_launch(PkgList *db, char ***sel, int *count); // Fallback

// Fetch
int fetch_pkg(const char *name, const char *url, const char *dest);

/* --- Pacman Wrapper --- */
int pac_exec(char **args, int n);
int pac_install(char **pkgs, int n, bool asdep);
int pac_remove(char **pkgs, int n, bool cas);

#endif