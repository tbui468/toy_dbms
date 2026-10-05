#ifndef DISK_SCHEDULER_H
#define DISK_SCHEDULER_H

#include "disk_manager.h"
#include <pthread.h>
#include <stdbool.h>

typedef struct {
    bool        is_write;
    char       *data;          /* BUSTUB_PAGE_SIZE buffer */
    page_id_t   page_id;
    /* simple completion: caller can wait on a condition or just poll */
    volatile bool done;
    bool        success;
} DiskRequest;

typedef struct {
    DiskManager *dm;
    /* simple bounded queue */
    DiskRequest **queue;
    size_t        q_cap;
    size_t        q_head, q_tail, q_size;
    pthread_mutex_t q_lock;
    pthread_cond_t  q_not_empty;
    pthread_cond_t  q_not_full;
    pthread_t       worker;
    bool            running;
} DiskScheduler;

DiskScheduler *disk_scheduler_create(DiskManager *dm);
void           disk_scheduler_destroy(DiskScheduler *ds);
void           disk_scheduler_schedule(DiskScheduler *ds, DiskRequest *reqs, size_t n);

#endif
