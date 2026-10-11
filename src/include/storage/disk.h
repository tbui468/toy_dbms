#ifndef DISK_H
#define DISK_H

#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include "storage/page.h"

struct request {
  bool complete;
  bool write;
  page_id_t page_id;
  char *page_data;
};

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

//bool request_wait(struct request *request);

void queue_init(struct queue *queue, int capacity);
void queue_destroy(struct queue *queue);
void queue_push(struct queue *queue, struct request *request);
struct request* queue_pop(struct queue *queue);
void queue_close(struct queue *queue);

void disk_init(struct disk *disk, int request_capacity, const char *path);
void disk_destroy(struct disk *disk);
void disk_schedule(struct disk *disk, struct request *request);


#endif // DISK_H
