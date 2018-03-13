#ifndef LOCKING_H
#define LOCKING_H

#include <stdbool.h>

typedef volatile bool lock_t;

// #define lock_release(l) {lock_release_(l); printf("rel %s %d\r\n", __FILE__, __LINE__);}
// #define lock_acquire(l) (lock_acquire_(l) && printf("acq %s %d\r\n", __FILE__, __LINE__))
#define lock_release(l) lock_release_(l)
#define lock_acquire(l) lock_acquire_(l)

static inline void lock_release_(lock_t *lock)
{
  *lock = false;
}

static inline bool lock_acquire_(lock_t *lock)
{
  if (*lock)
    return false;

  *lock = true;
  return true;
}

static inline bool lock_acquire2(lock_t *lock, lock_t *lock2)
{
  if (!lock_acquire(lock))
    return false;

  if (!lock_acquire(lock2))
  {
    lock_release(lock);
    return false;
  }

  return true;
}

static inline bool lock_is_free(lock_t *lock)
{
  return !(*lock);
}

#endif /* end of include guard: LOCKING_H */
