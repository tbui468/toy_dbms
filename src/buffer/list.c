#include "buffer/list.h"
#include "util/util.h"

void list_init(struct list *list, uint32_t capacity) {
  assert(capacity > 0);
  list->capacity = capacity;
  list->nodes = malloc(list->capacity * sizeof(struct node));
  if (!list->nodes) FATAL();

  for (frame_id_t i = 0; i < capacity; i++) {
    list->nodes[i].next = INVALID_FRAME_ID;
    list->nodes[i].prev = INVALID_FRAME_ID;
  }
  list->size = 0;

  list->front = INVALID_FRAME_ID;
  list->back = INVALID_FRAME_ID;
  latch_init(&list->latch);
}

void list_free(struct list *list) {
  free(list->nodes);
  latch_destroy(&list->latch);
}

uint32_t list_size(struct list *list) {
  latch_lock(&list->latch);
  uint32_t size = list->size;
  latch_unlock(&list->latch);
  return size;
}

void list_append(struct list *list, frame_id_t id) {
  assert(id < list->capacity);
  assert(!list_contains(list, id));

  latch_lock(&list->latch);

  struct node *node = &list->nodes[id];
  node->next = INVALID_FRAME_ID;

  if (list->size == 0) {
    list->back = id;
    list->front = id;
    node->prev = INVALID_FRAME_ID;
    list->size++;
    latch_unlock(&list->latch);
    return;
  }

  struct node *prev = &list->nodes[list->back];
  prev->next = id;
  node->prev = list->back;
  list->back = id;

  list->size++;
  latch_unlock(&list->latch);
  return;
}

static void list_do_erase(struct list *list, frame_id_t id) {
  struct node *node = &list->nodes[id];

  if (node->prev != INVALID_FRAME_ID) {
    struct node *prev = &list->nodes[node->prev];
    prev->next = node->next;
  } else {
    list->front = node->next;
  }

  if (node->next != INVALID_FRAME_ID) {
    struct node *next = &list->nodes[node->next];
    next->prev = node->prev;
  } else {
    list->back = node->prev;
  }

  node->next = INVALID_FRAME_ID;
  node->prev = INVALID_FRAME_ID;

  list->size--;
}

void list_erase(struct list *list, frame_id_t id) {
  latch_lock(&list->latch);
  list_do_erase(list, id);
  latch_unlock(&list->latch);
}

frame_id_t list_pop_front(struct list *list) {
  latch_lock(&list->latch);
  if (list->front == INVALID_FRAME_ID) {
    latch_unlock(&list->latch);
    return INVALID_FRAME_ID;
  }
  frame_id_t value = list->front;
  list_do_erase(list, list->front); 
  latch_unlock(&list->latch);
  return value;
}

bool list_contains(struct list *list, frame_id_t id) {
  latch_lock(&list->latch);

  struct node *node = &list->nodes[id];
  bool unused = node->next == INVALID_FRAME_ID &&
                node->prev == INVALID_FRAME_ID &&
                list->front != id;

  latch_unlock(&list->latch);
  return !unused;
}
