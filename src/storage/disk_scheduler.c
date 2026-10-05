#include "disk_scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define Q_CAPACITY 1024

static void *worker_thread(void *arg) {
    DiskScheduler *ds = arg;
    while (ds->running) {
        pthread_mutex_lock(&ds->q_lock);
        while (ds->q_size == 0 && ds->running)
            pthread_cond_wait(&ds->q_not_empty, &ds->q_lock);
        if (!ds->running && ds->q_size == 0) {
            pthread_mutex_unlock(&ds->q_lock);
            break;
        }
        DiskRequest *req = ds->queue[ds->q_head];
        ds->q_head = (ds->q_head + 1) % ds->q_cap;
        ds->q_size--;
        pthread_cond_signal(&ds->q_not_full);
        pthread_mutex_unlock(&ds->q_lock);

        if (req->is_write)
            disk_manager_write_page(ds->dm, req->page_id, req->data);
        else
            disk_manager_read_page (ds->dm, req->page_id, req->data);
        req->success = true;
        req->done    = true;
    }
    return NULL;
}

DiskScheduler *disk_scheduler_create(DiskManager *dm) {
    DiskScheduler *ds = calloc(1, sizeof(*ds));
    ds->dm     = dm;
    ds->q_cap  = Q_CAPACITY;
    ds->queue  = calloc(ds->q_cap, sizeof(DiskRequest *));
    pthread_mutex_init(&ds->q_lock, NULL);
    pthread_cond_init (&ds->q_not_empty, NULL);
    pthread_cond_init (&ds->q_not_full,  NULL);
    ds->running = true;
    pthread_create(&ds->worker, NULL, worker_thread, ds);
    return ds;
}

void disk_scheduler_destroy(DiskScheduler *ds) {
    pthread_mutex_lock(&ds->q_lock);
    ds->running = false;
    pthread_cond_broadcast(&ds->q_not_empty);
    pthread_mutex_unlock(&ds->q_lock);
    pthread_join(ds->worker, NULL);
    free(ds->queue);
    pthread_mutex_destroy(&ds->q_lock);
    pthread_cond_destroy(&ds->q_not_empty);
    pthread_cond_destroy(&ds->q_not_full);
    free(ds);
}

void disk_scheduler_schedule(DiskScheduler *ds, DiskRequest *reqs, size_t n) {
    for (size_t i = 0; i < n; i++) {
        pthread_mutex_lock(&ds->q_lock);
        while (ds->q_size == ds->q_cap)
            pthread_cond_wait(&ds->q_not_full, &ds->q_lock);
        ds->queue[ds->q_tail] = &reqs[i];
        ds->q_tail = (ds->q_tail + 1) % ds->q_cap;
        ds->q_size++;
        pthread_cond_signal(&ds->q_not_empty);
        pthread_mutex_unlock(&ds->q_lock);
    }
}
