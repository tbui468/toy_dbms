#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

#define INVALID_NODE_OFFSET UINT32_MAX
typedef uint32_t node_id_t;

struct node {
  node_id_t next;
  node_id_t prev;
};

struct list {
  node_id_t capacity;
  node_id_t size;

  node_id_t front;
  node_id_t back;

  struct node *nodes;

  pthread_mutex_t latch;
};


void list_init(struct list *list, int capacity);
void list_free(struct list *list);
int list_size(struct list *list);
// returns internal offset to allow quick removal
void list_append(struct list *list, node_id_t id);
void list_erase(struct list *list, node_id_t id);
size_t list_pop_front(struct list *list);
bool list_contains(struct list *list, node_id_t id);

#endif // LIST_H
