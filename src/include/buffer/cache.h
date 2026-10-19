#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>
#include "buffer/guard.h"
#include "buffer/replacer.h"
#include "storage/page.h"
#include "storage/disk.h"

struct cache {
  uint32_t capacity;
  struct replacer replacer;
  struct guard header;
  struct disk *disk;
  struct list free_frames;
  struct table page_frame_table;
  struct frame_header *frame_headers;
  uint8_t *frame_buffers;
};

void cache_init(struct cache *cache, struct disk *disk, uint32_t capacity);
//initialize the cache without initalizing meta page guard to simplify testing of internal API
void cache_init_basic(struct cache *cache, struct disk *disk, uint32_t capacity);
void cache_destroy(struct cache *cache);
void cache_destroy_basic(struct cache *cache);
frame_id_t cache_find_free_frame(struct cache *cache);
void cache_fill_frame(struct cache *cache, frame_id_t frame_id, page_id_t page_id);
bool cache_lookup_page(struct cache *cache, page_id_t page_id, frame_id_t *frame_id);
void cache_flush_page(struct cache *cache, page_id_t page_id);
struct frame_header* cache_frame_header(struct cache *cache, frame_id_t frame_id);
uint8_t* cache_frame_buffer(struct cache *cache, frame_id_t frame_id);

page_id_t cache_alloc_page(struct cache *cache);
void cache_free_page(struct cache *cache, page_id_t page_id);

#endif // CACHE_H
