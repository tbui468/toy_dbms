#ifndef DISK_H
#define DISK_H

#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include "storage/page.h"

enum request_type {
  RT_READ,
  RT_WRITE,
  RT_FLUSH
};

struct request {
  pthread_mutex_t latch;  
  pthread_cond_t complete_cond;
  bool complete;
  enum request_type type;
  page_id_t page_id;
  char *page_data;
};

#define QUEUE_CAPACITY 8

struct queue {
  size_t capacity;
  size_t size;
  size_t front;
  size_t back;
  void **values; 
  pthread_mutex_t latch;  
  pthread_cond_t non_empty;
  pthread_cond_t non_full;
  bool closed;
};

struct disk {
  struct queue requests;
  pthread_t request_handler;
  int fd;
};

void request_init(struct request *request, enum request_type type, page_id_t page_id);
void request_acquire_data(struct request *request, char **ptr);
char *request_release_data(struct request *request);
void request_destroy(struct request *request);
void request_wait(struct request *request);

void queue_init(struct queue *queue);
void queue_destroy(struct queue *queue);
void queue_push(struct queue *queue, struct request *request);
struct request* queue_pop(struct queue *queue);
void queue_close(struct queue *queue);

void disk_init(struct disk *disk, const char *path);
void disk_destroy(struct disk *disk);
void disk_schedule(struct disk *disk, struct request *request);


#endif // DISK_H
