#include "storage/disk_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

static size_t allocate_page(struct disk_manager *dm) {
    if (dm->free_count > 0) {
        return dm->free_slots[--dm->free_count];
    }
    if (dm->num_pages + 1 >= dm->page_capacity) {
        dm->page_capacity *= 2;
        ftruncate(dm->db_fd, (off_t)(dm->page_capacity + 1) * PAGE_SIZE);
        dm->page_offsets = realloc(dm->page_offsets,
                                   dm->page_capacity * sizeof(size_t));
    }
    size_t off = dm->num_pages * PAGE_SIZE;
    dm->num_pages++;
    return off;
}

void disk_manager_init(struct disk_manager *dm, const char *db_file) {
    dm->db_path  = strdup(db_file);
    char logbuf[512];
    snprintf(logbuf, sizeof(logbuf), "%s.log", db_file);
    dm->log_path = strdup(logbuf);

    dm->db_fd  = open(db_file,  O_RDWR | O_CREAT, 0644);
    dm->log_fd = open(logbuf,   O_RDWR | O_CREAT | O_APPEND, 0644);
    if (dm->db_fd < 0 || dm->log_fd < 0) {
        perror("open");
        return;
    }

    dm->page_capacity = 16;
    ftruncate(dm->db_fd, (off_t)(dm->page_capacity + 1) * PAGE_SIZE);
    dm->page_offsets = calloc(dm->page_capacity, sizeof(size_t));
    dm->free_cap     = 64;
    dm->free_slots   = malloc(dm->free_cap * sizeof(size_t));
    pthread_mutex_init(&dm->lock, NULL);
}

void disk_manager_shutdown(struct disk_manager *dm) {
    if (!dm) return;
    close(dm->db_fd);
    close(dm->log_fd);
    free(dm->db_path);
    free(dm->log_path);
    free(dm->page_offsets);
    free(dm->free_slots);
    pthread_mutex_destroy(&dm->lock);
    free(dm);
}

void disk_manager_write_page(struct disk_manager *dm, page_id_t page_id, const char *data) {
    pthread_mutex_lock(&dm->lock);
    size_t offset;
    if (page_id < (page_id_t)dm->page_capacity && dm->page_offsets[page_id] != 0) {
        offset = dm->page_offsets[page_id];
    } else {
        offset = allocate_page(dm);
        if ((size_t)page_id >= dm->page_capacity) {
            /* grow map if necessary – omitted for brevity */
        }
        dm->page_offsets[page_id] = offset;
    }
    pwrite(dm->db_fd, data, PAGE_SIZE, (off_t)offset);
    dm->num_writes++;
    pthread_mutex_unlock(&dm->lock);
}

void disk_manager_read_page(struct disk_manager *dm, page_id_t page_id, char *data) {
    pthread_mutex_lock(&dm->lock);
    size_t offset = 0;
    if (page_id < (page_id_t)dm->page_capacity)
        offset = dm->page_offsets[page_id];
    if (offset == 0) {
        offset = allocate_page(dm);
        dm->page_offsets[page_id] = offset;
    }
    ssize_t n = pread(dm->db_fd, data, PAGE_SIZE, (off_t)offset);
    if (n < PAGE_SIZE)
        memset(data + n, 0, PAGE_SIZE - n);
    pthread_mutex_unlock(&dm->lock);
}

void disk_manager_delete_page(struct disk_manager *dm, page_id_t page_id) {
    pthread_mutex_lock(&dm->lock);
    if (page_id >= (page_id_t)dm->page_capacity || dm->page_offsets[page_id] == 0) {
        pthread_mutex_unlock(&dm->lock);
        return;
    }
    if (dm->free_count == dm->free_cap) {
        dm->free_cap *= 2;
        dm->free_slots = realloc(dm->free_slots, dm->free_cap * sizeof(size_t));
    }
    dm->free_slots[dm->free_count++] = dm->page_offsets[page_id];
    dm->page_offsets[page_id] = 0;
    dm->num_deletes++;
    pthread_mutex_unlock(&dm->lock);
}

void disk_manager_write_log(struct disk_manager *dm, const char *data, int size) {
    if (size == 0) return;
    write(dm->log_fd, data, size);
    fsync(dm->log_fd);
    dm->num_flushes++;
}

int disk_manager_read_log(struct disk_manager *dm, char *data, int size, int offset) {
    struct stat st;
    fstat(dm->log_fd, &st);
    if (offset >= st.st_size) return 0;
    ssize_t n = pread(dm->log_fd, data, size, offset);
    if (n < size) memset(data + n, 0, size - n);
    return 1;
}
