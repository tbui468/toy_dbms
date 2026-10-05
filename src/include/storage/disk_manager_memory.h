/* disk_manager_memory.h / .c  – in-memory variants */

#ifndef DISK_MANAGER_MEMORY_H
#define DISK_MANAGER_MEMORY_H

#include "disk_manager.h"   /* for page_id_t, BUSTUB_PAGE_SIZE */

typedef struct {
    char   *memory;
    size_t  capacity;
    int     num_writes;
} DiskManagerMemory;

DiskManagerMemory *disk_manager_memory_create(size_t capacity);
void disk_manager_memory_destroy(DiskManagerMemory *dm);
void disk_manager_memory_write_page(DiskManagerMemory *dm, page_id_t id, const char *data);
void disk_manager_memory_read_page (DiskManagerMemory *dm, page_id_t id, char *data);

/* Unlimited version (simplified – no latency simulation) */
typedef struct {
    char          **pages;      /* array of page buffers */
    size_t          capacity;
    pthread_mutex_t lock;
    int             num_writes;
} DiskManagerUnlimitedMemory;

DiskManagerUnlimitedMemory *disk_manager_unlimited_create(void);
void disk_manager_unlimited_destroy(DiskManagerUnlimitedMemory *dm);
void disk_manager_unlimited_write_page(DiskManagerUnlimitedMemory *dm, page_id_t id, const char *data);
void disk_manager_unlimited_read_page (DiskManagerUnlimitedMemory *dm, page_id_t id, char *data);

#endif
