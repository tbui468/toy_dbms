#include "buffer/cache.h"
#include "util/util.h"

void cache_init(struct cache *cache, struct disk *disk, uint32_t capacity) {
  cache_init_basic(cache, disk, capacity);

  struct guard guard;
  guard_init(&guard, GT_WRITE, cache, META_PAGE_ID);
  struct meta_page *header = meta_page_mut(&guard);
  meta_page_init(header);
  guard_destroy(&guard);
}

void cache_init_basic(struct cache *cache, struct disk *disk, uint32_t capacity) {
  cache->capacity = capacity;
  cache->disk = disk;
  list_init(&cache->free_frames, capacity);
  table_init(&cache->page_frame_table, capacity);
  for (frame_id_t i = 0; i < capacity; i++) {
    list_append(&cache->free_frames, i);
  }
  replacer_init(&cache->replacer, capacity);

  cache->frame_headers = malloc(sizeof(struct frame_header) * capacity);
  if (!cache->frame_headers) FATAL();

  cache->frame_buffers = malloc(PAGE_SIZE * capacity);
  if (!cache->frame_buffers) FATAL();

  for (uint32_t i = 0; i < capacity; i++) {
    frame_header_init(&cache->frame_headers[i], &cache->frame_buffers[i * PAGE_SIZE]);
  }

  latch_init(&cache->latch);
}

void cache_destroy(struct cache *cache) {
  for (uint32_t i = 0; i < cache->capacity; i++) {
    struct frame_header *header = &cache->frame_headers[i];
    if (header->dirty) {
      cache_flush_page(cache, header->page_id);
    }
  }

  replacer_free(&cache->replacer);
  list_free(&cache->free_frames);
  table_free(&cache->page_frame_table);
  for (uint32_t i = 0; i < cache->capacity; i++) {
    frame_header_destroy(&cache->frame_headers[i]);
  }
  free(cache->frame_headers);
  free(cache->frame_buffers);
  latch_destroy(&cache->latch);
}

frame_id_t cache_find_free_frame(struct cache *cache) {
  return list_pop_front(&cache->free_frames);
}

void cache_fill_frame(struct cache *cache, frame_id_t frame_id, page_id_t page_id) {
  struct request request;
  request_init(&request, RT_READ, page_id);
  uint8_t *page_data = cache_frame_buffer(cache, frame_id);
  request_acquire_data(&request, page_data);
  disk_schedule(cache->disk, &request);

  //update frame header bookkeeping
  struct frame_header *header = cache_frame_header(cache, frame_id);
  header->dirty = false;
  header->pins = 0;
  header->page_data = page_data;
  header->page_id = page_id;
  table_put(&cache->page_frame_table, page_id, frame_id);

  //wait until disk finishes
  request_wait(&request);
  page_data = request_release_data(&request);
  // page_data doesn't need to be used since frame buffer already has data read in at this point
  request_destroy(&request);
}

bool cache_lookup_page(struct cache *cache, page_id_t page_id, frame_id_t *frame_id) {
  return table_get(&cache->page_frame_table, page_id, frame_id);
}

void cache_flush_page(struct cache *cache, page_id_t page_id) {
  assert(page_id != INVALID_PAGE_ID);
  struct request request;
  request_init(&request, RT_WRITE, page_id);
  frame_id_t frame_id;
  bool found = cache_lookup_page(cache, page_id, &frame_id);
  assert(found); 
  uint8_t *page_data = cache_frame_buffer(cache, frame_id);
  request_acquire_data(&request, page_data);
  disk_schedule(cache->disk, &request);
  request_wait(&request);
  page_data = request_release_data(&request);
  // page_data doesn't need to be used since frame buffer already has data read in at this point
  request_destroy(&request);
}

struct frame_header* cache_frame_header(struct cache *cache, frame_id_t frame_id) {
  return &cache->frame_headers[frame_id];
}

uint8_t* cache_frame_buffer(struct cache *cache, frame_id_t frame_id) {
  return &cache->frame_buffers[frame_id * PAGE_SIZE];
}

uint32_t cache_page_pin_count(struct cache *cache, page_id_t page_id) {
  frame_id_t frame_id;
  bool result = table_get(&cache->page_frame_table, page_id, &frame_id);
  return result ? cache_frame_header(cache, frame_id)->pins : 0;
}

page_id_t cache_alloc_page(struct cache *cache) {
  struct guard guard;
  guard_init(&guard, GT_WRITE, cache, META_PAGE_ID);
  struct meta_page *header = meta_page_mut(&guard);
  page_id_t page_id = meta_page_alloc_page(header);
  guard_destroy(&guard);
  return page_id;
}

void cache_free_page(struct cache *cache, page_id_t page_id) {
  struct guard guard;
  guard_init(&guard, GT_WRITE, cache, META_PAGE_ID);
  struct meta_page *header = meta_page_mut(&guard);
  meta_page_free_page(header, page_id);
  guard_destroy(&guard);
}
