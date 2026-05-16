/**
 * @file download.c
 * @author BoringPenguin
 * - Uses libcurl's 'multi_socket' API (one of the fastest public APIs i found).
 * - Integrates directly with Linux 'epoll' for O(1) event notification.
 * - Supports HTTP/2 Multiplexing & TCP Fast Open.
 * - Single-threaded async I/O (no context switching overhead).
 */
#include "../yaarp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <curl/curl.h>

// Configuration
#define MAX_SIMULTANEOUS 16      /* Max parallel connections */
#define TIMEOUT_MS 30000         /* Global timeout */
#define BUFFER_SIZE (256 * 1024) /* 256KB Write Buffers */
#define EPOLL_MAX_EVENTS 128

// State Machine
typedef enum {
    STATE_INIT,
    STATE_RUNNING,
    STATE_DONE,
    STATE_FAILED
} TaskState;

typedef struct {
    char url[2048];
    char path[MAX_PATH];
    FILE *fp;
    
    /* Stats (Atomic-ish updates) */
    long long downloaded_bytes;
    long long total_bytes;
    
    TaskState state;
    CURL *easy_handle;
    int epoll_fd;               /* Ref to the manager's epoll */
    char error_buf[CURL_ERROR_SIZE];
} DownloadTask;

typedef struct {
    CURLM *multi_handle;
    int epoll_fd;
    int timer_fd;               /* For curl timeouts */
    int still_running;
    bool stop_flag;
} DownloadManager;

// Forward Declarations
static int msock_callback(CURL *e, curl_socket_t s, int what, void *userp, void *socketp);
static int timer_callback(CURLM *multi, long timeout_ms, void *userp);
static void event_loop(DownloadManager *mgr);
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata);

// Implementation 

/**
 * Initialize the High-Speed Engine
 */
DownloadManager* fetch_init(void) {
    DownloadManager *mgr = xmalloc(sizeof(DownloadManager));
    
    mgr->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if(mgr->epoll_fd < 0) {
        log_err("Failed to create epoll instance");
        exit(1);
    }

    /* Create a timerfd for curl timeouts */
    mgr->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    
    mgr->multi_handle = curl_multi_init();
    
    /* Tuning for SPEED */
    curl_multi_setopt(mgr->multi_handle, CURLMOPT_SOCKETFUNCTION, msock_callback);
    curl_multi_setopt(mgr->multi_handle, CURLMOPT_SOCKETDATA, mgr);
    curl_multi_setopt(mgr->multi_handle, CURLMOPT_TIMERFUNCTION, timer_callback);
    curl_multi_setopt(mgr->multi_handle, CURLMOPT_TIMERDATA, mgr);
    
    /* Pipeline/Multiplex settings */
    curl_multi_setopt(mgr->multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, MAX_SIMULTANEOUS);
    curl_multi_setopt(mgr->multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 4); 
    
    mgr->still_running = 0;
    mgr->stop_flag = false;
    
    return mgr;
}

/**
 * Add a download task to the engine
 */
void fetch_add(DownloadManager *mgr, const char *url, const char *dest_path) {
    DownloadTask *task = xcalloc(1, sizeof(DownloadTask));
    
    strncpy(task->url, url, sizeof(task->url)-1);
    strncpy(task->path, dest_path, sizeof(task->path)-1);
    task->state = STATE_INIT;
    
    /* Open file for binary write */
    task->fp = fopen(dest_path, "wb");
    if(!task->fp) {
        log_err("Cannot open %s", dest_path);
        free(task);
        return;
    }
    
    /* Configure Easy Handle */
    CURL *easy = curl_easy_init();
    task->easy_handle = easy;
    
    curl_easy_setopt(easy, CURLOPT_URL, url);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, task);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, task);
    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, task->error_buf);
    curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);
    
    /* Performance Optimizations */
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);       /* Redirects */
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(easy, CURLOPT_TCP_FASTOPEN, 1L);          /* TFO */
    curl_easy_setopt(easy, CURLOPT_TCP_NODELAY, 1L);           /* No Nagle */
    curl_easy_setopt(easy, CURLOPT_BUFFERSIZE, BUFFER_SIZE);
    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS); /* H2 */
    curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");       /* Enable compression */
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, TIMEOUT_MS / 1000);
    
    /* Connect to Manager */
    curl_easy_setopt(easy, CURLOPT_OPENSOCKETDATA, mgr);
    curl_easy_setopt(easy, CURLOPT_CLOSESOCKETDATA, mgr);

    curl_multi_add_handle(mgr->multi_handle, easy);
}

/**
 * The Main Loop: Drives transfers using Epoll
 * This replaces the inefficient curl_multi_perform polling loop.
 */
int fetch_run(DownloadManager *mgr) {
    event_loop(mgr);
    return 0; /* Return 0 for success, could return fail count */
}

void fetch_cleanup(DownloadManager *mgr) {
    curl_multi_cleanup(mgr->multi_handle);
    close(mgr->epoll_fd);
    close(mgr->timer_fd);
    free(mgr);
}

// Internal Callbacks

/*
 * CURL CALLBACK: Socket Action
 * Tells us when a socket is created or changes state.
 * We register/deregister it from our epoll loop immediately.
 */
static int msock_callback(CURL *e, curl_socket_t s, int what, void *userp, void *socketp) {
    (void)e; (void)socketp;
    DownloadManager *mgr = (DownloadManager*)userp;
    
    struct epoll_event ev;
    ev.data.fd = s;
    ev.events = 0;

    if(what == CURL_POLL_IN || what == CURL_POLL_INOUT) ev.events |= EPOLLIN;
    if(what == CURL_POLL_OUT || what == CURL_POLL_INOUT) ev.events |= EPOLLOUT;

    if(what == CURL_POLL_REMOVE) {
        epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, s, NULL);
    } else {
        if(socketp == NULL) { /* New socket */
            int *fdp = xmalloc(sizeof(int));
            *fdp = s;
            curl_multi_assign(mgr->multi_handle, s, fdp);
            ev.data.ptr = fdp; /* Store ptr to clean up later */
        }
        
        /* Mod or Add */
        int op = socketp ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_ctl(mgr->epoll_fd, op, s, &ev);
    }
    return 0;
}

/*
 * CURL CALLBACK: Timeout
 * Sets the OS timer to wake up epoll exactly when curl needs action.
 */
static int timer_callback(CURLM *multi, long timeout_ms, void *userp) {
    (void)multi;
    DownloadManager *mgr = (DownloadManager*)userp;
    
    struct itimerspec its;
    memset(&its, 0, sizeof(struct itimerspec));
    
    if(timeout_ms < 0) {
        /* Disable timer */
        timerfd_settime(mgr->timer_fd, 0, &its, NULL);
    } else {
        if(timeout_ms == 0) timeout_ms = 1; /* 0 means call ASAP, set 1ms */
        
        its.it_value.tv_sec = timeout_ms / 1000;
        its.it_value.tv_nsec = (timeout_ms % 1000) * 1000000;
        timerfd_settime(mgr->timer_fd, 0, &its, NULL);
    }
    return 0;
}

/*
 * DATA CALLBACK: Write to Disk
 * Runs in the context of the main loop.
 */
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    DownloadTask *t = (DownloadTask*)userdata;
    size_t written = fwrite(ptr, size, nmemb, t->fp);
    t->downloaded_bytes += written;
    return written;
}

/*
 * THE EVENT LOOP
 * Blocks until work is done or timeout.
 */
static void event_loop(DownloadManager *mgr) {
    struct epoll_event events[EPOLL_MAX_EVENTS];
    
    /* Add timer fd to epoll */
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = mgr->timer_fd;
    epoll_ctl(mgr->epoll_fd, EPOLL_CTL_ADD, mgr->timer_fd, &ev);

    int code;
    (void)code;
    do {
        int numfds = epoll_wait(mgr->epoll_fd, events, EPOLL_MAX_EVENTS, 10000);
        
        if(numfds < 0 && errno != EINTR) break;

        /* 1. Handle Timeouts (Curl says "do something now") */
        for(int i=0; i<numfds; i++) {
            if(events[i].data.fd == mgr->timer_fd) {
                uint64_t exp;
                read(mgr->timer_fd, &exp, sizeof(exp)); /* Consume */
                curl_multi_socket_action(mgr->multi_handle, CURL_SOCKET_TIMEOUT, 0, &mgr->still_running);
                break;
            }
        }

        /* 2. Handle Socket Activity */
        for(int i=0; i<numfds; i++) {
            if(events[i].data.fd == mgr->timer_fd) continue;
            
            int action = 0;
            if(events[i].events & EPOLLIN) action |= CURL_CSELECT_IN;
            if(events[i].events & EPOLLOUT) action |= CURL_CSELECT_OUT;
            if(events[i].events & (EPOLLERR | EPOLLHUP)) action |= CURL_CSELECT_ERR;
            
            int fd = events[i].data.fd;
            
            /* Note: Ideally we check if data.ptr holds our fd pointer logic,
               but here we assume data.fd is sufficient for lookup */
            curl_multi_socket_action(mgr->multi_handle, fd, action, &mgr->still_running);
        }

        /* 3. Check completed downloads */
        CURLMsg *msg;
        int msgs_left;
        while((msg = curl_multi_info_read(mgr->multi_handle, &msgs_left))) {
            if(msg->msg == CURLMSG_DONE) {
                CURL *e = msg->easy_handle;
                DownloadTask *t;
                curl_easy_getinfo(e, CURLINFO_PRIVATE, &t);
                
                if(msg->data.result != CURLE_OK) {
                    log_err("Fail: %s -> %s", t->url, t->error_buf);
                    t->state = STATE_FAILED;
                    unlink(t->path); /* Clean partial */
                } else {
                    t->state = STATE_DONE;
                    double speed;
                    curl_easy_getinfo(e, CURLINFO_SPEED_DOWNLOAD_T, &speed);
                    log_info("Done: %s (%.2f KB/s)", t->url, speed/1024);
                }
                
                if(t->fp) fclose(t->fp);
                curl_multi_remove_handle(mgr->multi_handle, e);
                curl_easy_cleanup(e);
                free(t);
            }
        }

    } while(mgr->still_running && !mgr->stop_flag);
}