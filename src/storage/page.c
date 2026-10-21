#include "storage/page.h"
#include "buffer/guard.h"
#include <assert.h>
#include <string.h>

struct meta_page* meta_page_mut(struct guard *guard) {
  assert(guard->type == GT_WRITE);
  return (struct meta_page*) guard->frame_header->page_data;
}
const struct meta_page* meta_page_immut(struct guard *guard) {
  assert(guard->type == GT_WRITE || guard->type == GT_READ);
  return (const struct meta_page*) guard->frame_header->page_data;
}

void meta_page_init(struct meta_page *page) {
  memset(page->used_bits, 0, PAGE_SIZE);
  page->used_bits[0] = 1; //first bit set for metapage allocation
}

page_id_t meta_page_alloc_page(struct meta_page *page) {
  page->used_bits[0] |= 1; // meta page is always allocated - this is a bit of a hack

  for (int i = 0; i < PAGE_SIZE; i++) {
    if (page->used_bits[i] == UINT8_MAX) continue;
    
    page_id_t byte_offset = (page_id_t) i * 8;
    page_id_t bit_offset = 0;
    while ((page->used_bits[i] >> bit_offset) & 1) {
      bit_offset++;
    }
    
    uint8_t mask = 1 << bit_offset;
    page->used_bits[i] |= mask; 

    return byte_offset + bit_offset;
  }

  return INVALID_PAGE_ID;
}
void meta_page_free_page(struct meta_page *page, page_id_t page_id) {
  int bit_offset = (int) page_id % 8;
  int byte_offset = (int) page_id / 8;

  uint8_t mask = 1 << bit_offset;
  mask = ~mask;
  page->used_bits[byte_offset] &= mask; 
}
