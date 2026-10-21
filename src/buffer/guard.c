#include "buffer/guard.h"
#include "util/util.h"
#include "buffer/frame.h"
#include "buffer/cache.h"
#include <stddef.h>
#include <assert.h>

void guard_init(struct guard *guard, enum guard_type type, struct cache *cache, page_id_t page_id) {
  guard->type = type;
  guard->cache = cache;

  latch_lock(&cache->latch);

  frame_id_t frame_id;
  if (!cache_lookup_page(cache, page_id, &frame_id)) {
    if ((frame_id = cache_find_free_frame(cache)) == INVALID_FRAME_ID) {
      frame_id = replacer_evict(&cache->replacer);
      if (frame_id == INVALID_FRAME_ID) assert(false); //TODO guard_init should return error code

      struct frame_header *header = cache_frame_header(cache, frame_id);
      if (header->dirty) {
        cache_flush_page(cache, header->page_id);        
      }
    }

    cache_fill_frame(cache, frame_id, page_id);
  }

  guard->frame_header = cache_frame_header(cache, frame_id);
  latch_unlock(&cache->latch);

  if (type == GT_READ) {
    rwlatch_rlock(&guard->frame_header->latch);
  } else {
    assert(type == GT_WRITE);
    rwlatch_wlock(&guard->frame_header->latch);
  }

  latch_lock(&cache->latch);

  guard->frame_header->pins++;
  if (guard->frame_header->pins > 0) {
    replacer_fix(&cache->replacer, frame_id); 
  }
  latch_unlock(&cache->latch);
}

void guard_destroy(struct guard *guard) {
  latch_lock(&guard->cache->latch);
  guard->frame_header->pins--;
  if (guard->frame_header->pins == 0) {
    frame_id_t frame_id = (frame_id_t) (guard->frame_header - guard->cache->frame_headers);
    replacer_unfix(&guard->cache->replacer, frame_id); 
  }
  latch_unlock(&guard->cache->latch);

  rwlatch_unlock(&guard->frame_header->latch);
}

const uint8_t* guard_raw_immut(struct guard *guard) {
  assert(guard->type == GT_READ || guard->type == GT_WRITE);
  return (const uint8_t*) guard->frame_header->page_data;
}

uint8_t* guard_raw_mut(struct guard *guard) {
  assert(guard->type == GT_WRITE);
  guard->frame_header->dirty = true;
  return guard->frame_header->page_data;
}
