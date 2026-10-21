#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#define FATAL() { printf("%d: %s\n", __LINE__, strerror(errno)); exit(1); }
#define FATAL_MSG(msg) { printf("%d: %s\n", __LINE__, #msg); exit(1); }


static inline void latch_init(pthread_mutex_t *latch) {
  if (pthread_mutex_init(latch, NULL) != 0) FATAL();
}

static inline void latch_destroy(pthread_mutex_t *latch) {
  if (pthread_mutex_destroy(latch) != 0) FATAL();
}

static inline void latch_lock(pthread_mutex_t *latch) {
  if (pthread_mutex_lock(latch) != 0) FATAL();
}
static inline void latch_unlock(pthread_mutex_t *latch) {
  if (pthread_mutex_unlock(latch) != 0) FATAL();
}

static inline void rwlatch_init(pthread_rwlock_t *latch) {
  if (pthread_rwlock_init(latch, NULL) != 0) FATAL();
}

static inline void rwlatch_destroy(pthread_rwlock_t *latch) {
  if (pthread_rwlock_destroy(latch) != 0) FATAL();
}

static inline void rwlatch_rlock(pthread_rwlock_t *latch) {
  if (pthread_rwlock_rdlock(latch) != 0) FATAL();
}
static inline void rwlatch_wlock(pthread_rwlock_t *latch) {
  if (pthread_rwlock_wrlock(latch) != 0) FATAL();
}
static inline void rwlatch_unlock(pthread_rwlock_t *latch) {
  if (pthread_rwlock_unlock(latch) != 0) FATAL();
}

#endif // UTIL_H
