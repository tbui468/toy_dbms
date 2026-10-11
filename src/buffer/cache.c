#include "buffer/bpm.h"

void bpm_init(struct bpm *bpm, int size) {
  bpm->size = size;
  bpm->next_page_id = 0;
  lru_init(&bpm->replacer, size);
}

void bpm_free(struct bpm *bpm) {
  lru_free(&bpm->replacer);
}

int bpm_size(const struct bpm *bpm) {
  return bpm->size;
}

page_id_t bpm_new_page(struct bpm *bpm) {
  return bpm->next_page_id++;
}
