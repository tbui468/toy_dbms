#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

#include "buffer/frame.h"

struct node {
  frame_id_t next;
  frame_id_t prev;
};

struct list {
  uint32_t capacity;
  uint32_t size;
  frame_id_t front;
  frame_id_t back;
  struct node *nodes;
  pthread_mutex_t latch;
};


void list_init(struct list *list, uint32_t capacity);
void list_free(struct list *list);
uint32_t list_size(struct list *list);
void list_append(struct list *list, frame_id_t id);
void list_erase(struct list *list, frame_id_t id);
frame_id_t list_pop_front(struct list *list);
bool list_contains(struct list *list, frame_id_t id);

#endif // LIST_H
