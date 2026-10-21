#include "buffer/frame.h"
#include "util/util.h"

void frame_header_init(struct frame_header *header, uint8_t *buffer) {
  header->dirty = false;
  header->pins = 0;
  header->page_data = buffer;
  header->page_id = INVALID_PAGE_ID;
  rwlatch_init(&header->latch);
}

void frame_header_destroy(struct frame_header *header) {
  rwlatch_destroy(&header->latch);
}
