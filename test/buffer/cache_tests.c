#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "buffer/cache.h"
#include "util/test.h"
#include "util/util.h"
#include "storage/disk.h"


static void write_page(int fd, page_id_t page_id, const uint8_t *page_data) {
  size_t written = 0;
  while (written < PAGE_SIZE) {
    off_t offset = (off_t) page_id * PAGE_SIZE + (off_t) written;
    ssize_t result = pwrite(fd, page_data + written, PAGE_SIZE - written, offset);
    if (result == -1) {
      if (errno == EINTR) continue;
      FATAL();
    }
    written += (size_t) result;
  }
}

static void read_page(int fd, page_id_t page_id, uint8_t *page_data) {
  size_t read = 0;
  while (read < PAGE_SIZE) {
    off_t offset = (off_t) page_id * PAGE_SIZE + (off_t) read;
    ssize_t result = pread(fd, page_data + read, PAGE_SIZE - read, offset);
    if (result == -1) {
      if (errno == EINTR) continue;
      FATAL();
    }

    if (result == 0) FATAL_MSG("EOF before reading entire page");

    read += (size_t) result;
  }
}

static void write_dummy_file(const char *path) {
  int fd = open(path, O_CREAT | O_RDWR, 0644);
  uint8_t *data = malloc(PAGE_SIZE);

  memset(data, 'a', PAGE_SIZE);
  write_page(fd, 0, data);

  memset(data, 'b', PAGE_SIZE);
  write_page(fd, 1, data);

  memset(data, 'c', PAGE_SIZE);
  write_page(fd, 2, data);

  memset(data, 'd', PAGE_SIZE);
  write_page(fd, 3, data);

  free(data);

  close(fd);
}

bool cache_init_destroy(void) {
  struct disk disk;
  disk_init(&disk, "test.db");
  struct cache cache;
  cache_init(&cache, &disk, 128);

  disk_destroy(&disk);
  cache_destroy(&cache);
  return true;
}

bool cache_alloc_free_pages(void) {
  struct disk disk;
  disk_init(&disk, "test.db");
  struct cache cache;
  cache_init_basic(&cache, &disk, 128);

  page_id_t pages[8];
  for (int i = 0; i < 8; i++) {
    pages[i] = cache_alloc_page(&cache);
    ASSERT_EQ(pages[i] != META_PAGE_ID, true);
    for (int j = 0; j < i; j++) {
      ASSERT_EQ(pages[i] != pages[j], true);
    }
  }

  disk_destroy(&disk);
  cache_destroy(&cache);
  return true;
}

bool cache_alloc_frames(void) {
  struct disk disk;
  disk_init(&disk, "test.db");
  struct cache cache;
  cache_init_basic(&cache, &disk, 128);

  for (frame_id_t i = 0; i < 128; i++) {
    ASSERT_EQ(cache_find_free_frame(&cache), i);
  }

  ASSERT_EQ(cache_find_free_frame(&cache), INVALID_FRAME_ID);

  disk_destroy(&disk);
  cache_destroy_basic(&cache);
  return true;
}

bool cache_pages_cached(void) {
  const char *path = "test.db";
  write_dummy_file(path);

  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 128);

  page_id_t page0 = 0;
  page_id_t page1 = 1;
  page_id_t page2 = 2;
  frame_id_t result_frame_id;
  ASSERT_EQ(cache_lookup_page(&cache, page0, &result_frame_id), false);
  ASSERT_EQ(cache_lookup_page(&cache, page1, &result_frame_id), false);
  ASSERT_EQ(cache_lookup_page(&cache, page2, &result_frame_id), false);
  frame_id_t frame_id = cache_find_free_frame(&cache);
  cache_fill_frame(&cache, frame_id, page1);  
  ASSERT_EQ(cache_lookup_page(&cache, page0, &result_frame_id), false);
  ASSERT_EQ(cache_lookup_page(&cache, page1, &result_frame_id), true);
  ASSERT_EQ(result_frame_id == frame_id, true);
  ASSERT_EQ(cache_lookup_page(&cache, page2, &result_frame_id), false);

  disk_destroy(&disk);
  cache_destroy_basic(&cache);
  return true;
}

bool cache_disk_copied_into_buffers(void) {
  const char *path = "test.db";
  write_dummy_file(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 128);

  page_id_t page_ids[] = { 1, 3, 0, 2 }; //randomize order

  for (int i = 0; i < 4; i++) {
    frame_id_t frame_id = cache_find_free_frame(&cache);
    cache_fill_frame(&cache, frame_id, page_ids[i]);
  }

  for (page_id_t page_id = 0; page_id < 4; page_id++) {
    frame_id_t frame_id;
    ASSERT_TRUE(cache_lookup_page(&cache, page_id, &frame_id));
    uint8_t *buffer = cache_frame_buffer(&cache, frame_id);
    for (int i = 0; i < PAGE_SIZE; i++) {
      ASSERT_EQ(buffer[i], 'a' + page_id);
    }
  }

  disk_destroy(&disk);
  cache_destroy_basic(&cache);
  return true;
}

bool cache_flush_page_test(void) {
  const char *path = "test.db";
  write_dummy_file(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 128);

  for (page_id_t i = 0; i < 4; i++) {
    frame_id_t frame_id = cache_find_free_frame(&cache);
    cache_fill_frame(&cache, frame_id, i);
    
    uint8_t *buffer = cache_frame_buffer(&cache, frame_id);
    memset(buffer, '0' + (int) i, PAGE_SIZE);
    if (i % 2 == 0) {
      cache_flush_page(&cache, i);
    }
  }

  uint8_t *buf = malloc(PAGE_SIZE);
  int fd = open(path, O_CREAT | O_RDWR, 0644);
  read_page(fd, 0, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], '0');
  }
  read_page(fd, 1, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], 'b');
  }
  read_page(fd, 2, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], '2');
  }
  read_page(fd, 3, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], 'd');
  }

  free(buf);
  disk_destroy(&disk);
  cache_destroy_basic(&cache);
  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  //RUN_TEST(cache_init_destroy);
  //RUN_TEST(cache_alloc_free_pages);
  RUN_TEST(cache_alloc_frames);
  RUN_TEST(cache_pages_cached);
  RUN_TEST(cache_disk_copied_into_buffers);
  RUN_TEST(cache_flush_page_test);

  /*
  RUN_TEST(lru_init_free);
  RUN_TEST(lru_first_access);
  RUN_TEST(lru_unfix_order);
  RUN_TEST(lru_evict_frame);
  RUN_TEST(lru_evict_and_reuse);
  RUN_TEST(lru_scale);
  */
 
  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
