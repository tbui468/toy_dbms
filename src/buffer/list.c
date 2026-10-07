#include "buffer/list.h"


static void list_link_freelist(struct list *list, int from, int to) {
  list->freelist = from;
  for (int i = from; i < to; i++) {
    list->nodes[i].next = i == to - 1 ? 
                            INVALID_NODE_OFFSET : 
                            i + 1;
    list->nodes[i].prev = INVALID_NODE_OFFSET;
  }
}

void list_init(struct list *list, int default_capacity) {
  assert(default_capacity > 0);
  list->capacity = default_capacity;
  list->nodes = malloc(list->capacity * sizeof(struct node));
  if (!list->nodes) {
    exit(1);
  }
  list->size = 0;
  list_link_freelist(list, 0, list->capacity);

  list->front = INVALID_NODE_OFFSET;
  list->back = INVALID_NODE_OFFSET;
  pthread_mutex_init(&list->latch, NULL);
}

void list_free(struct list *list) {
  free(list->nodes);
  pthread_mutex_destroy(&list->latch);
}

int list_size(struct list *list) {
  pthread_mutex_lock(&list->latch);
  size_t size = list->size;
  pthread_mutex_unlock(&list->latch);
  return size;
}

// returns internal offset to allow quick removal
size_t list_append(struct list *list, node_id_t id) {
  pthread_mutex_lock(&list->latch);
  if (list->freelist == INVALID_NODE_OFFSET) {
    assert(list->size == list->capacity);
    size_t prev_cap = list->capacity;
    list->capacity *= 2;
    list->nodes = realloc(list->nodes, list->capacity * sizeof(struct node));
    if (!list->nodes) {
      exit(1);
    }

    list_link_freelist(list, prev_cap, list->capacity);
  }

  size_t off = list->freelist;
  list->freelist = list->nodes[list->freelist].next;

  struct node *node = &list->nodes[off];
  node->id = id;
  node->next = INVALID_NODE_OFFSET;

  if (list->size == 0) {
    list->back = off;
    list->front = off;
    node->prev = INVALID_NODE_OFFSET;
    list->size++;
    pthread_mutex_unlock(&list->latch);
    return off; 
  }

  struct node *prev = &list->nodes[list->back];
  prev->next = off;
  node->prev = list->back;
  list->back = off;

  list->size++;
  pthread_mutex_unlock(&list->latch);
  return off;
}

static void list_do_erase(struct list *list, size_t off) {
  struct node *node = &list->nodes[off];

  if (node->prev != INVALID_NODE_OFFSET) {
    struct node *prev = &list->nodes[node->prev];
    prev->next = node->next;
  } else {
    list->front = node->next;
  }

  if (node->next != INVALID_NODE_OFFSET) {
    struct node *next = &list->nodes[node->next];
    next->prev = node->prev;
  } else {
    list->back = node->prev;
  }

  node->next = list->freelist;
  list->freelist = off;

  list->size--;
}

void list_erase(struct list *list, size_t off) {
  pthread_mutex_lock(&list->latch);
  list_do_erase(list, off);
  pthread_mutex_unlock(&list->latch);
}

size_t list_pop_front(struct list *list) {
  pthread_mutex_lock(&list->latch);
  if (list->front == INVALID_NODE_OFFSET) {
    pthread_mutex_unlock(&list->latch);
    return INVALID_NODE_OFFSET;
  }
  size_t value = list->nodes[list->front].id; 
  list_do_erase(list, list->front); 
  pthread_mutex_unlock(&list->latch);
  return value;
}

bool list_contains(struct list *list, node_id_t id) {
  pthread_mutex_lock(&list->latch);
  size_t off = list->front;
  while (off != INVALID_NODE_OFFSET) {
    struct node *node = &list->nodes[off];
    if (node->id == id) {
      pthread_mutex_unlock(&list->latch);
      return true;
    }

    off = node->next;
  }

  pthread_mutex_unlock(&list->latch);
  return false;
}
