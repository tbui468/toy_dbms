#include "buffer/guard.h"
#include <stddef.h>
#include <assert.h>

void guard_init(struct guard *guard, enum guard_type type, page_id_t page_id, struct cache *cache) {
  guard->type = type;
  guard->id = page_id;
  guard->data = NULL; // TODO this acutally needs to be pointed to buffers
  guard->cache = cache;

  /*
  frame_id_t frame_id;
  if (!cache_contains_page(cache, page_id, &frame_id)) {
    if ((frame_id = cache_get_free_frame(cache)) == INVALID_FRAME_ID) {
      //TODO evict unpinned frame - otherwise crash for now
      //     if contents of page in evicted frame is dirty, flush to disk
      //     assign frame_id to this newly evicted frame
    }
    struct frame *frame = cache_get_frame(cache, frame_id);
    //TODO initialize frame (including reading data from disk)
  }

  struct frame *frame = cache_get_frame(cache, frame_id);
  frame_increment_pin(frame);

  // TODO cache checks if page_id already exists in cache - need to use table here to map page_id -> frame_id
  //        if so, get data from frame and increment pin on it
  //        else allocate new frame, read data from disk - but if a new frame cannot be allocated, must run eviction algorithm

  //page->data = get cache to give frame with loaded data here
  */

  assert(false);
}

void guard_destroy(struct guard *guard) {
  guard = guard; // TODO actually do something  
}
