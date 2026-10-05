#include "disk_manager_memory.h"
#include <stdlib.h>
#include <string.h>

DiskManagerMemory *disk_manager_memory_create(size_t capacity) {
    DiskManagerMemory *dm = malloc(sizeof(*dm));
    dm->capacity = capacity;
    dm->memory   = calloc(capacity, BUSTUB_PAGE_SIZE);
    dm->num_writes = 0;
    return dm;
}

void disk_manager_memory_destroy(DiskManagerMemory *dm) {
    free(dm->memory);
    free(dm);
}

void disk_manager_memory_write_page(DiskManagerMemory *dm, page_id_t id, const char *data) {
    if ((size_t)id >= dm->capacity) return; /* or abort */
    memcpy(dm->memory + (size_t)id * BUSTUB_PAGE_SIZE, data, BUSTUB_PAGE_SIZE);
    dm->num_writes++;
}

void disk_manager_memory_read_page(DiskManagerMemory *dm, page_id_t id, char *data) {
    if ((size_t)id >= dm->capacity) {
        memset(data, 0, BUSTUB_PAGE_SIZE);
        return;
    }
    memcpy(data, dm->memory + (size_t)id * BUSTUB_PAGE_SIZE, BUSTUB_PAGE_SIZE);
}

/* ---- Unlimited ---- */
DiskManagerUnlimitedMemory *disk_manager_unlimited_create(void) {
    DiskManagerUnlimitedMemory *dm = calloc(1, sizeof(*dm));
    dm->capacity = 64;
    dm->pages = calloc(dm->capacity, sizeof(char *));
    pthread_mutex_init(&dm->lock, NULL);
    return dm;
}

void disk_manager_unlimited_destroy(DiskManagerUnlimitedMemory *dm) {
    for (size_t i = 0; i < dm->capacity; i++)
        free(dm->pages[i]);
    free(dm->pages);
    pthread_mutex_destroy(&dm->lock);
    free(dm);
}

static void ensure_page(DiskManagerUnlimitedMemory *dm, page_id_t id) {
    if ((size_t)id >= dm->capacity) {
        size_t newcap = id + 1;
        dm->pages = realloc(dm->pages, newcap * sizeof(char *));
        for (size_t i = dm->capacity; i < newcap; i++)
            dm->pages[i] = NULL;
        dm->capacity = newcap;
    }
    if (!dm->pages[id])
        dm->pages[id] = calloc(1, BUSTUB_PAGE_SIZE);
}

void disk_manager_unlimited_write_page(DiskManagerUnlimitedMemory *dm, page_id_t id, const char *data) {
    pthread_mutex_lock(&dm->lock);
    ensure_page(dm, id);
    memcpy(dm->pages[id], data, BUSTUB_PAGE_SIZE);
    dm->num_writes++;
    pthread_mutex_unlock(&dm->lock);
}

void disk_manager_unlimited_read_page(DiskManagerUnlimitedMemory *dm, page_id_t id, char *data) {
    pthread_mutex_lock(&dm->lock);
    ensure_page(dm, id);
    memcpy(data, dm->pages[id], BUSTUB_PAGE_SIZE);
    pthread_mutex_unlock(&dm->lock);
}
