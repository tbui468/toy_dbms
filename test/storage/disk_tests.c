#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "storage/disk.h"
#include "util/test.h"

bool queue_init_free(void) {
  struct queue queue;
  queue_init(&queue);
  ASSERT_EQ((int) queue.size, 0);
  ASSERT_EQ((int) queue.capacity, 8);
  queue_destroy(&queue);
  return true;
}

bool queue_push_test(void) {
  struct queue queue;
  queue_init(&queue);
  struct request r;
  for (int i = 0; i < 8; i++) {
    queue_push(&queue, &r);
    ASSERT_EQ((int) queue.size, i + 1);
  }

  queue_destroy(&queue);
  return true;
}

bool queue_pop_test(void) {
  struct queue queue;
  queue_init(&queue);
  struct request requests[8];
  for (int i = 0; i < 8; i++) {
    requests[i].page_id = i;
    queue_push(&queue, &requests[i]);
  }
  for (int i = 0; i < 8; i++) {
    struct request *r = queue_pop(&queue);
    ASSERT_EQ(r != NULL, true);
    ASSERT_EQ((int) r->page_id, i);
  }
  queue_destroy(&queue);
  return true;
}

#define COUNT (256L * 256L)

struct data {
  struct queue *queue;
  int id;
  struct request *requests;
};

void *concurrency_producer(void *arg) {
  struct data *data = (struct data *) arg;
  struct queue *queue = data->queue;
  int id = data->id;
  struct request *requests = data->requests;
  for (int i = 0; i < COUNT; i++) {
    if (i % 2 != id) continue;

    requests[i].page_id = i;
    queue_push(queue, &requests[i]);
  }
}

void *concurrency_consumer(void *arg) {
  struct data *data = (struct data *) arg;
  struct queue *queue = data->queue;
  struct request *requests = data->requests;
  size_t total = (COUNT * (COUNT - 1)) / 2;
  while (total != 0) {
    struct request *r = queue_pop(queue);
    total -= r->page_id;
  }
}

bool queue_concurrency(void) {
  struct queue queue;
  queue_init(&queue);
  
  pthread_t t0, t1, t2;

  struct request *requests = malloc(sizeof(struct request) * COUNT);

  struct data d0;
  d0.queue = &queue;
  d0.id = 0;
  d0.requests = requests;

  struct data d1;
  d1.queue = &queue;
  d1.id = 1;
  d1.requests = requests;

  struct data d2;
  d2.queue = &queue;
  //d2.id not used
  d2.requests = requests;
 
  pthread_create(&t0, NULL, concurrency_producer, &d0);
  pthread_create(&t1, NULL, concurrency_producer, &d1); 
  pthread_create(&t2, NULL, concurrency_consumer, &d2); 

  pthread_join(t0, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  ASSERT_EQ((int) queue.size, 0);

  free(requests);
  queue_destroy(&queue);
  return true;
}

bool disk_init_free(void) {
  struct disk disk;
  disk_init(&disk, "test.db");
  disk_destroy(&disk);
  return true;
}

#define REQUEST_COUNT 26

bool disk_schedule_requests(void) {
  struct disk disk;
  disk_init(&disk, "test.db");

  char *data[REQUEST_COUNT];
  for (int i = 0; i < REQUEST_COUNT; i++) {
    data[i] = malloc(PAGE_SIZE);
    memset(data[i], 'a' + i, PAGE_SIZE);
  }

  struct request requests[REQUEST_COUNT];
  for (int i = 0; i < REQUEST_COUNT; i++) {
    request_init(&requests[i], RT_WRITE, i);
    request_acquire_data(&requests[i], &data[i]);
    disk_schedule(&disk, &requests[i]);
  }
  
  for (int i = 0; i < REQUEST_COUNT; i++) {
    request_wait(&requests[i]);
    data[i] = request_release_data(&requests[i]);
    request_destroy(&requests[i]);
  }

  for (int i = 0; i < REQUEST_COUNT; i++) {
    memset(data[i], 0, PAGE_SIZE);
  }

  //read back pages to ensure same data
  for (int i = 0; i < REQUEST_COUNT; i++) {
    request_init(&requests[i], RT_READ, i);
    request_acquire_data(&requests[i], &data[i]);
    disk_schedule(&disk, &requests[i]);
  }

  for (int i = 0; i < REQUEST_COUNT; i++) {
    request_wait(&requests[i]);
    data[i] = request_release_data(&requests[i]);
    request_destroy(&requests[i]);
  }

  for (int i = 0; i < REQUEST_COUNT; i++) {
    int expected = 'a' + i;
    for (int j = 0; j < PAGE_SIZE; j++) {
      int actual = data[i][j];
      ASSERT_EQ(actual, expected);
    }
  }


  disk_destroy(&disk);
  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(queue_init_free);
  RUN_TEST(queue_push_test);
  RUN_TEST(queue_pop_test);
  RUN_TEST(queue_concurrency);

  RUN_TEST(disk_init_free);
  RUN_TEST(disk_schedule_requests);

  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
