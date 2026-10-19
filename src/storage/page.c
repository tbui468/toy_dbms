#include "storage/page.h"
#include "buffer/guard.h"
#include <assert.h>
#include <stdio.h>

struct meta_page* meta_page_mut(struct guard *guard) {
  assert(guard->type == GT_WRITE);
  return (struct meta_page*) guard->data;
}
const struct meta_page* meta_page_immut(struct guard *guard) {
  assert(guard->type == GT_WRITE || guard->type == GT_READ);
  return (const struct meta_page*) guard->data;
}
page_id_t meta_page_alloc_page(struct meta_page *page) {
  page = page;
  assert(false);
  return INVALID_PAGE_ID;
}
void meta_page_free_page(struct meta_page *page, page_id_t page_id) {
  page = page;
  assert(false);
  printf("deleting page %d\n", page_id);
}
