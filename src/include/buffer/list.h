#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define INVALID_NODE_OFFSET SIZE_MAX
typedef size_t node_id_t;

struct node {
  size_t next;
  size_t prev;
  node_id_t id;
};

struct list {
  size_t capacity;
  size_t size;
  size_t freelist;  

  size_t front;
  size_t back;

  struct node *nodes;
};


void list_init(struct list *list, int default_capacity);
void list_free(struct list *list);
// returns internal offset to allow quick removal
size_t list_append(struct list *list, node_id_t id);
void list_erase(struct list *list, size_t off);
bool list_contains(struct list *list, node_id_t id);

#endif // LIST_H
