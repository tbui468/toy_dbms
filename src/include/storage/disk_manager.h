#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#define PAGE_SIZE 4096
typedef int32_t page_id_t;

struct disk_manager {
    char     *db_path;
    char     *log_path;
    int       db_fd;
    int       log_fd;
    size_t    page_capacity;
    size_t   *page_offsets;      /* page_id -> offset, 0 = free/unused */
    size_t   *free_slots;
    size_t    free_count;
    size_t    free_cap;
    size_t    num_pages;
    int       num_writes;
    int       num_deletes;
    int       num_flushes;
    pthread_mutex_t lock;
};

void  disk_manager_init(struct disk_manager *dm, const char *db_file);
void  disk_manager_shutdown(struct disk_manager *dm);
void  disk_manager_write_page(struct disk_manager *dm, page_id_t page_id, const char *data);
void  disk_manager_read_page (struct disk_manager *dm, page_id_t page_id, char *data);
void  disk_manager_delete_page(struct disk_manager *dm, page_id_t page_id);
void  disk_manager_write_log (struct disk_manager *dm, const char *data, int size);
int   disk_manager_read_log  (struct disk_manager *dm, char *data, int size, int offset);

#endif
