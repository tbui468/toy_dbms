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

static void write_dummy_file_large(const char *path) {
  int fd = open(path, O_CREAT | O_RDWR, 0644);
  uint8_t *data = malloc(PAGE_SIZE);

  for (page_id_t i = 0; i < 8; i++) {
    memset(data, 'a' + (int) i, PAGE_SIZE);
    write_page(fd, i, data);
  }

  free(data);
  close(fd);
}

static void write_dummy_meta_page(const char *path) {
  int fd = open(path, O_CREAT | O_RDWR, 0644);
  uint8_t *data = malloc(PAGE_SIZE);

  memset(data, 0, PAGE_SIZE);
  write_page(fd, 0, data);

  free(data);
  close(fd);
}

bool cache_alloc_free_pages(void) {
  const char *path = "test.db";
  write_dummy_meta_page(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init(&cache, &disk, 128);

  page_id_t pages[15];
  for (int i = 0; i < 15; i++) {
    pages[i] = cache_alloc_page(&cache);
    ASSERT_EQ(pages[i] != META_PAGE_ID, true);
    for (int j = 0; j < i; j++) {
      ASSERT_EQ(pages[i] != pages[j], true);
    }
  }

  struct guard meta;
  guard_init(&meta, GT_READ, &cache, META_PAGE_ID);
  const struct meta_page *header = meta_page_immut(&meta);
  ASSERT_EQ(header->used_bits[0],  UINT8_MAX);
  ASSERT_EQ(header->used_bits[1],  UINT8_MAX);
  for (int i = 2; i < PAGE_SIZE; i++) {
    ASSERT_EQ(header->used_bits[i], 0);
  }
  guard_destroy(&meta);

  for (int i = 0; i < 15; i++) {
    cache_free_page(&cache, pages[i]);
  }

  guard_init(&meta, GT_READ, &cache, META_PAGE_ID);
  header = meta_page_immut(&meta);
  ASSERT_EQ(header->used_bits[0],  1);
  for (int i = 1; i < PAGE_SIZE; i++) {
    ASSERT_EQ(header->used_bits[i], 0);
  }
  guard_destroy(&meta);

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
  cache_destroy(&cache);
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
  cache_destroy(&cache);
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
  cache_destroy(&cache);
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
  cache_destroy(&cache);
  return true;
}

bool guard_basic_test(void) {
  const char *path = "test.db";
  write_dummy_file(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 128);

  for (page_id_t page_id = 0; page_id < 4; page_id++) {
    struct guard guard;
    guard_init(&guard, GT_READ, &cache, page_id);
    const uint8_t *data = guard_raw_immut(&guard);
    for (int i = 0; i < PAGE_SIZE; i++) {
      ASSERT_EQ(data[i], 'a' + page_id);
    }
    guard_destroy(&guard);
  }

  for (page_id_t page_id = 0; page_id < 4; page_id++) {
    struct guard guard;
    guard_init(&guard, GT_WRITE, &cache, page_id);
    uint8_t *data = guard_raw_mut(&guard);
    for (int i = 0; i < PAGE_SIZE; i++) {
      data[i] = '0' + (uint8_t) page_id;
    }
    guard_destroy(&guard);
    cache_flush_page(&cache, page_id);
  }

  //verify that 4 pages contain ASCII 0, 1, 2 and 3
  uint8_t *buf = malloc(PAGE_SIZE);
  int fd = open(path, O_CREAT | O_RDWR, 0644);
  for (page_id_t page_id = 0; page_id < 4; page_id++) {
    read_page(fd, page_id, buf);
    for (int i = 0; i < PAGE_SIZE; i++) {
      ASSERT_EQ(buf[i], '0' + page_id);
    }
  }

  close(fd);
  free(buf);
  cache_destroy(&cache);
  disk_destroy(&disk);
  return true;
}

bool guard_pin_count_test(void) {
  const char *path = "test.db";
  write_dummy_file(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 4);

  struct guard guards[8];
  struct guard wguard;

  ASSERT_EQ(cache_page_pin_count(&cache, 0), 0);
  ASSERT_EQ(cache_page_pin_count(&cache, 1), 0);

  for (int i = 0; i < 8; i++) {
    guard_init(&guards[i], GT_READ, &cache, 0);
    const uint8_t *data = guard_raw_immut(&guards[i]);
    for (int i = 0; i < PAGE_SIZE; i++) {
      ASSERT_EQ(data[i], 'a');
    }
  }
  guard_init(&wguard, GT_WRITE, &cache, 1);

  ASSERT_EQ(cache_page_pin_count(&cache, 0), 8);
  ASSERT_EQ(cache_page_pin_count(&cache, 1), 1);

  for (int i = 0; i < 8; i++) {
    guard_destroy(&guards[i]);
  }
  guard_destroy(&wguard);

  ASSERT_EQ(cache_page_pin_count(&cache, 0), 0);
  ASSERT_EQ(cache_page_pin_count(&cache, 1), 0);

  cache_destroy(&cache);
  disk_destroy(&disk);
  return true;
}

bool cache_eviction_test(void) {
  const char *path = "test.db";
  write_dummy_file(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 4);

  struct guard guards[8];
  struct guard guard;
  page_id_t pinned_page = 3;
  guard_init(&guard, GT_READ, &cache, pinned_page);
  ASSERT_EQ(cache_page_pin_count(&cache, pinned_page), 1);

  for (page_id_t i = 0; i < 8; i++) {
    guard_init(&guards[i], GT_READ, &cache, i);
    guard_destroy(&guards[i]);
    ASSERT_EQ(cache_page_pin_count(&cache, pinned_page), 1);
  }

  for (page_id_t i = 0; i < 8; i++) {
    if (i == pinned_page) continue;
    guard_init(&guards[i], GT_WRITE, &cache, i);
    guard_destroy(&guards[i]);
  }

  ASSERT_EQ(cache_page_pin_count(&cache, pinned_page), 1);

  disk_destroy(&disk);
  cache_destroy(&cache);
  return true;
}

bool cache_dirty_pages_flushed_test(void) {
  const char *path = "test.db";
  write_dummy_file(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 4);

  struct guard write0, write1, read0;

  guard_init(&write0, GT_WRITE, &cache, 0);
  guard_init(&write1, GT_WRITE, &cache, 1);
  guard_init(&read0, GT_READ, &cache, 2);
  uint8_t *data = guard_raw_mut(&write0);
  memset(data, '0', PAGE_SIZE);
  const uint8_t *immut_data = guard_raw_immut(&write1);
  immut_data = guard_raw_immut(&read0);
  immut_data = immut_data; //silence unused warning

  guard_destroy(&write0);
  guard_destroy(&write1);
  guard_destroy(&read0);

  for (page_id_t i = 0; i < 8; i++) {
    struct guard guard;
    guard_init(&guard, GT_READ, &cache, i);
    guard_destroy(&guard);
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
    ASSERT_EQ(buf[i], 'c');
  }

  close(fd);
  free(buf);
  disk_destroy(&disk);
  cache_destroy(&cache);
  return true;
}

#define ITER_COUNT 256 * 256
#define READER_COUNT 6

void *concurrent_reader(void *arg) {
  struct cache *cache = (struct cache *) arg;
  struct guard guard;
  for (page_id_t i = 0; i < ITER_COUNT; i++) {
    page_id_t page_id = i % 8;
    guard_init(&guard, GT_READ, cache, page_id);

    const uint8_t *data = guard_raw_immut(&guard);
    for (int j = 0; j < PAGE_SIZE; j++) {
      ASSERT_EQ(data[0], data[j]);
    }

    uint32_t pin_count = cache_page_pin_count(cache, page_id);
    ASSERT_TRUE(pin_count > 0 && pin_count <= READER_COUNT);
    guard_destroy(&guard);

    pin_count = cache_page_pin_count(cache, page_id);
    ASSERT_TRUE(pin_count < READER_COUNT);
  }
  return NULL;
}

struct writer_data {
  struct cache *cache;
  int id;
};

void *concurrent_writer(void *arg) {
  struct writer_data *data = (struct writer_data*) arg;
  struct cache *cache = data->cache;
  int id = data->id;
  
  struct guard guard;
  for (page_id_t i = 0; i < ITER_COUNT; i++) {
    page_id_t page_id = i % 8;
    guard_init(&guard, GT_WRITE, cache, page_id);

    uint8_t *data = guard_raw_mut(&guard);
    for (int j = 0; j < PAGE_SIZE; j++) {
      data[j] = (uint8_t) ('0' + id % 2);
    }

    uint32_t pin_count = cache_page_pin_count(cache, page_id);
    ASSERT_TRUE(pin_count == 1);
    guard_destroy(&guard);

    pin_count = cache_page_pin_count(cache, page_id);
    ASSERT_TRUE(pin_count <= READER_COUNT);
  }
  return NULL;
}

bool cache_concurrent_test(void) {
  const char *path = "test.db";
  write_dummy_file_large(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 8);

  pthread_t threads[8];
  for (int i = 0; i < READER_COUNT; i++) {
    pthread_create(&threads[i], NULL, concurrent_reader, &cache);
  }

  struct writer_data data[2];
  data[0].cache = &cache;
  data[0].id = 0;
  data[1].cache = &cache;
  data[1].id = 1;
  for (int i = READER_COUNT; i < 8; i++) {
    pthread_create(&threads[i], NULL, concurrent_writer, &data[i - READER_COUNT]);
  }

  for (int i = 0; i < 8; i++) {
    pthread_join(threads[i], NULL);
  }

  cache_destroy(&cache);
  disk_destroy(&disk);
  return true;
}

bool cache_flush_dirty_on_destroy_test(void) {
  const char *path = "test.db";
  write_dummy_file(path);
  struct disk disk;
  disk_init(&disk, path);
  struct cache cache;
  cache_init_basic(&cache, &disk, 4);

  for (page_id_t i = 0; i < 4; i++) {
    struct guard guard;
    guard_init(&guard, GT_WRITE, &cache, i);
    uint8_t *data = guard_raw_mut(&guard);
    memset(data, '0' + (int) i, PAGE_SIZE);
    guard_destroy(&guard);
  }

  cache_destroy(&cache);
  disk_destroy(&disk);

  uint8_t *buf = malloc(PAGE_SIZE);
  int fd = open(path, O_RDWR, 0644);
  read_page(fd, 0, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], '0');
  }
  read_page(fd, 1, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], '1');
  }
  read_page(fd, 2, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], '2');
  }
  read_page(fd, 3, buf);
  for (int i = 0; i < PAGE_SIZE; i++) {
    ASSERT_EQ(buf[i], '3');
  }

  close(fd);
  free(buf);

  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(cache_alloc_free_pages);
  RUN_TEST(cache_alloc_frames);
  RUN_TEST(cache_pages_cached);
  RUN_TEST(cache_disk_copied_into_buffers);
  RUN_TEST(cache_flush_page_test);
  RUN_TEST(cache_eviction_test);
  RUN_TEST(cache_dirty_pages_flushed_test);
  RUN_TEST(cache_concurrent_test);
  RUN_TEST(cache_flush_dirty_on_destroy_test);

  RUN_TEST(guard_basic_test);
  RUN_TEST(guard_pin_count_test);
 
  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
