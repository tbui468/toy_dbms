#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

typedef uint32_t page_id_t;
#define PAGE_SIZE 8192
#define INVALID_PAGE_ID UINT32_MAX
#define META_PAGE_ID 0

struct guard;

struct meta_page {
  uint8_t used_bits[PAGE_SIZE];
};

_Static_assert(sizeof(struct meta_page) == PAGE_SIZE, "struct meta_page must be same as PAGE_SIZE");

struct meta_page* meta_page_mut(struct guard *guard);
const struct meta_page* meta_page_immut(struct guard *guard);
void meta_page_init(struct meta_page *page);
page_id_t meta_page_alloc_page(struct meta_page *page);
void meta_page_free_page(struct meta_page *page, page_id_t page_id);

#endif // PAGE_H
