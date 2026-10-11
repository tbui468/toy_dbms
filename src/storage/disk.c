#include "storage/disk.h"
#include "util/util.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

void queue_init(struct queue *queue, int capacity) {
  queue->capacity = capacity;
  queue->size = 0;
  queue->front = 0;
  queue->back = 0;
  queue->closed = false;

  queue->values = malloc(queue->capacity * sizeof(void *));
  if (!queue->values) FATAL();

  int result = pthread_mutex_init(&queue->latch, NULL);
  if (result != 0) FATAL();

  result = pthread_cond_init(&queue->non_empty, NULL);
  if (result != 0) FATAL();

  result = pthread_cond_init(&queue->non_full, NULL);
  if (result != 0) FATAL();
}
void queue_destroy(struct queue *queue) {
  free(queue->values);

  int result = pthread_mutex_destroy(&queue->latch);
  if (result != 0) FATAL();

  result = pthread_cond_destroy(&queue->non_empty);
  if (result != 0) FATAL();

  result = pthread_cond_destroy(&queue->non_full);
  if (result != 0) FATAL();
}
void queue_push(struct queue *queue, struct request *request) {
  pthread_mutex_lock(&queue->latch);
  while (queue->size == queue->capacity && !queue->closed) {
    pthread_cond_wait(&queue->non_full, &queue->latch);
  }

  if (queue->closed) {
    pthread_mutex_unlock(&queue->latch);
    return;
  }

  queue->values[queue->back] = (void*) request;
  queue->back = queue->back + 1;
  if (queue->back >= queue->capacity) {
    queue->back = 0;
  }
  queue->size++;
  pthread_mutex_unlock(&queue->latch);
  pthread_cond_signal(&queue->non_empty);
}
struct request* queue_pop(struct queue *queue) {
  pthread_mutex_lock(&queue->latch);

  while (queue->size == 0 && !queue->closed) {
    pthread_cond_wait(&queue->non_empty, &queue->latch);
  }

  // if queue->size > 0, let the worker finish writing requests to disk before exiting
  if (queue->closed && queue->size == 0) {
    pthread_mutex_unlock(&queue->latch);
    return NULL;
  }

  struct request *request = queue->values[queue->front];
  queue->front = queue->front + 1;
  if (queue->front >= queue->capacity) {
    queue->front = 0;
  }
  queue->size--;
  pthread_mutex_unlock(&queue->latch);
  pthread_cond_signal(&queue->non_full);
  return request;
}

void queue_close(struct queue *queue) {
  pthread_mutex_lock(&queue->latch);
  queue->closed = true;
  pthread_mutex_unlock(&queue->latch);

  pthread_cond_broadcast(&queue->non_full);
  pthread_cond_broadcast(&queue->non_empty);
}

static void *handle_request(void *arg) {
  struct disk *disk = (struct disk*) arg;
  struct request *r;
  while (true) {
    struct request *r = queue_pop(&disk->requests);
    if (!r) {
      break;
    }
    printf("Handled page id :%ld\n", r->page_id);
    r->complete = true;
  }
}

void disk_init(struct disk *disk, int request_capacity, const char *path) {
  queue_init(&disk->requests, request_capacity);
  pthread_create(&disk->request_handler, NULL, handle_request, disk);
  disk->fd = open(path, O_CREAT | O_RDWR, 0644);
  if (disk->fd == -1) FATAL();
}

void disk_destroy(struct disk *disk) {
  queue_close(&disk->requests);
  pthread_join(disk->request_handler, NULL);
  queue_destroy(&disk->requests);
  close(disk->fd);
}

void disk_schedule(struct disk *disk, struct request *request) {
  //while (!queue_push(&disk->requests, request))
  //  ;
}

static void disk_write_page(struct disk *disk, page_id_t page_id, const char *page_data) {
  size_t written = 0;
  while (written < PAGE_SIZE) {
    ssize_t result = pwrite(disk->fd, page_data + written, PAGE_SIZE - written, page_id * PAGE_SIZE + written);
    if (result == -1) {
      if (errno == EINTR) continue;
      FATAL();
    }
    written += result;
  }
}

static void disk_read_page(struct disk *disk, page_id_t page_id, char *page_data) {
  size_t read = 0;
  while (read < PAGE_SIZE) {
    ssize_t result = pread(disk->fd, page_data + read, PAGE_SIZE - read, page_id * PAGE_SIZE + read);
    if (result == -1) {
      if (errno == EINTR) continue;
      FATAL();
    }

    if (result == 0) FATAL_MSG("EOF before reading entire page");

    read += result;
  }
}
