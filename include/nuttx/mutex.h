/****************************************************************************
 * include/nuttx/mutex.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_MUTEX_H
#define __INCLUDE_NUTTX_MUTEX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <nuttx/clock.h>
#include <nuttx/semaphore.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXMUTEX_NO_HOLDER      ((pid_t)-1)
#define NXMUTEX_INITIALIZER    {NXSEM_INITIALIZER(1, SEM_TYPE_MUTEX | \
                                SEM_PRIO_INHERIT), NXMUTEX_NO_HOLDER}
#define NXRMUTEX_INITIALIZER   {NXMUTEX_INITIALIZER, 0}

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

struct mutex_s
{
  sem_t sem;
  pid_t holder;
};

typedef struct mutex_s mutex_t;

struct rmutex_s
{
  mutex_t mutex;
  unsigned int count;
};

typedef struct rmutex_s rmutex_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: nxmutex_init
 *
 * Description:
 *   This function initializes the UNNAMED mutex. Following a
 *   successful call to nxmutex_init(), the mutex may be used in subsequent
 *   calls to nxmutex_lock(), nxmutex_unlock(), and nxmutex_trylock().  The
 *   mutex remains usable until it is destroyed.
 *
 * Parameters:
 *   mutex - Semaphore to be initialized
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *
 ****************************************************************************/

static inline int nxmutex_init(FAR mutex_t *mutex)
{
  int ret = _SEM_INIT(&mutex->sem, 0, 1);

  if (ret < 0)
    {
      return _SEM_ERRVAL(ret);
    }

  mutex->holder = NXMUTEX_NO_HOLDER;
  _SEM_SETPROTOCOL(&mutex->sem, SEM_TYPE_MUTEX | SEM_PRIO_INHERIT);
  return ret;
}

/****************************************************************************
 * Name: nxmutex_destroy
 *
 * Description:
 *   This function initializes the UNNAMED mutex. Following a
 *   successful call to nxmutex_init(), the mutex may be used in subsequent
 *   calls to nxmutex_lock(), nxmutex_unlock(), and nxmutex_trylock().  The
 *   mutex remains usable until it is destroyed.
 *
 * Parameters:
 *   mutex - Semaphore to be destroyed
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *
 ****************************************************************************/

static inline int nxmutex_destroy(FAR mutex_t *mutex)
{
  int ret = _SEM_DESTROY(&mutex->sem);

  if (ret < 0)
    {
      return _SEM_ERRVAL(ret);
    }

  mutex->holder = NXMUTEX_NO_HOLDER;
  return ret;
}

/****************************************************************************
 * Name: nxmutex_is_hold
 *
 * Description:
 *   This function check whether the caller hold the mutex
 *   referenced by 'mutex'.
 *
 * Parameters:
 *   mutex - mutex descriptor.
 *
 * Return Value:
 *
 ****************************************************************************/

static inline bool nxmutex_is_hold(FAR mutex_t *mutex)
{
  return mutex->holder == gettid();
}

/****************************************************************************
 * Name: nxmutex_is_locked
 *
 * Description:
 *   This function get the lock state the mutex referenced by 'mutex'.
 *
 * Parameters:
 *   mutex - mutex descriptor.
 *
 * Return Value:
 *
 ****************************************************************************/

static inline bool nxmutex_is_locked(FAR mutex_t *mutex)
{
  int cnt;
  int ret;

  ret = _SEM_GETVALUE(&mutex->sem, &cnt);

  return ret >= 0 && cnt < 1;
}

/****************************************************************************
 * Name: nxmutex_lock
 *
 * Description:
 *   This function attempts to lock the mutex referenced by 'mutex'.  The
 *   mutex is implemented with a semaphore, so if the semaphore value is
 *   (<=) zero, then the calling task will not return until it successfully
 *   acquires the lock.
 *
 * Parameters:
 *   mutex - mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 ****************************************************************************/

static inline int nxmutex_lock(FAR mutex_t *mutex)
{
  int ret;

  DEBUGASSERT(!nxmutex_is_hold(mutex));
  for (; ; )
    {
      /* Take the semaphore (perhaps waiting) */

      ret = _SEM_WAIT(&mutex->sem);
      if (ret >= 0)
        {
          mutex->holder = gettid();
          break;
        }

      ret = _SEM_ERRVAL(ret);
      if (ret != -EINTR && ret != -ECANCELED)
        {
          break;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: nxmutex_trylock
 *
 * Description:
 *   This function locks the mutex only if the mutex is currently not locked.
 *   If the mutex has been locked already, the call returns without blocking.
 *
 * Parameters:
 *   mutex - mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 *     -EINVAL - Invalid attempt to lock the mutex
 *     -EAGAIN - The mutex is not available.
 *
 ****************************************************************************/

static inline int nxmutex_trylock(FAR mutex_t *mutex)
{
  int ret;

  DEBUGASSERT(!nxmutex_is_hold(mutex));
  ret = _SEM_TRYWAIT(&mutex->sem);
  if (ret < 0)
    {
      return _SEM_ERRVAL(ret);
    }

  mutex->holder = gettid();
  return ret;
}

/****************************************************************************
 * Name: nxmutex_timedlock
 *
 * Description:
 *   This function attempts to lock the mutex .  If the mutex value
 *   is (<=) zero,then the calling task will not return until it
 *   successfully acquires the lock or timed out
 *
 * Input Parameters:
 *   mutex   - Mutex object
 *   timeout - The time when mutex lock timed out
 *
 * Returned Value:
 *   OK        The mutex successfully acquires
 *   EINVAL    The mutex argument does not refer to a valid mutex.  Or the
 *             thread would have blocked, and the abstime parameter specified
 *             a nanoseconds field value less than zero or greater than or
 *             equal to 1000 million.
 *   ETIMEDOUT The mutex could not be locked before the specified timeout
 *             expired.
 *   EDEADLK   A deadlock condition was detected.
 *
 ****************************************************************************/

static inline int nxmutex_timedlock(FAR mutex_t *mutex, unsigned int timeout)
{
  int ret;
  struct timespec now;
  struct timespec delay;
  struct timespec rqtp;

  clock_gettime(CLOCK_MONOTONIC, &now);
  clock_ticks2time(MSEC2TICK(timeout), &delay);
  clock_timespec_add(&now, &delay, &rqtp);

  /* Wait until we get the lock or until the timeout expires */

  do
    {
      ret = _SEM_CLOCKWAIT(&mutex->sem, CLOCK_MONOTONIC, &rqtp);
      if (ret < 0)
        {
          ret = _SEM_ERRVAL(ret);
        }
    }
  while (ret == -EINTR || ret == -ECANCELED);

  if (ret >= 0)
    {
      mutex->holder = gettid();
    }

  return ret;
}

/****************************************************************************
 * Name: nxmutex_unlock
 *
 * Description:
 *   This function attempts to unlock the mutex referenced by 'mutex'.
 *
 * Parameters:
 *   mutex - mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 * Assumptions:
 *   This function may be called from an interrupt handler.
 *
 ****************************************************************************/

static inline int nxmutex_unlock(FAR mutex_t *mutex)
{
  int ret;

  DEBUGASSERT(nxmutex_is_hold(mutex));

  mutex->holder = NXMUTEX_NO_HOLDER;

  ret = _SEM_POST(&mutex->sem);
  if (ret < 0)
    {
      return _SEM_ERRVAL(ret);
    }

  return ret;
}

/****************************************************************************
 * Name: nxmutex_reset
 *
 * Description:
 *   This function reset lock state.
 *
 * Parameters:
 *   mutex - mutex descriptor.
 *
 * Return Value:
 *
 ****************************************************************************/

static inline int nxmutex_reset(FAR mutex_t *mutex)
{
  int ret;

  ret = nxsem_reset(&mutex->sem, 1);
  if (ret >= 0)
    {
      mutex->holder = NXMUTEX_NO_HOLDER;
    }

  return ret;
}

/****************************************************************************
 * Name: nxmutex_breaklock
 *
 * Description:
 *   This function attempts to break the mutex
 *
 * Parameters:
 *   mutex   - Mutex descriptor.
 *   locked  - Is the mutex break success
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 ****************************************************************************/

static inline int nxmutex_breaklock(FAR mutex_t *mutex, FAR bool *locked)
{
  int ret = OK;

  *locked = false;
  if (nxmutex_is_hold(mutex))
    {
      ret = nxmutex_unlock(mutex);
      if (ret >= 0)
        {
          *locked = true;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: nxmutex_restorelock
 *
 * Description:
 *   This function attempts to restore the mutex.
 *
 * Parameters:
 *   mutex   - mutex descriptor.
 *   locked  - true: it's mean that the mutex is broke success
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure
 *
 ****************************************************************************/

static inline int nxmutex_restorelock(FAR mutex_t *mutex, bool locked)
{
  return locked ? nxmutex_lock(mutex) : OK;
}

/****************************************************************************
 * Name: nxrmutex_init
 *
 * Description:
 *   This function initializes the UNNAMED recursive mutex. Following a
 *   successful call to nxrmutex_init(), the recursive mutex may be used in
 *   subsequent calls to nxrmutex_lock(), nxrmutex_unlock(),
 *   and nxrmutex_trylock(). The recursive mutex remains usable
 *   until it is destroyed.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be initialized
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *
 ****************************************************************************/

static inline int nxrmutex_init(FAR rmutex_t *rmutex)
{
  rmutex->count = 0;
  return nxmutex_init(&rmutex->mutex);
}

/****************************************************************************
 * Name: nxrmutex_destroy
 *
 * Description:
 *   This function destroy the UNNAMED recursive mutex.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be destroyed
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *
 ****************************************************************************/

static inline int nxrmutex_destroy(FAR rmutex_t *rmutex)
{
  int ret = nxmutex_destroy(&rmutex->mutex);

  if (ret >= 0)
    {
      rmutex->count = 0;
    }

  return ret;
}

/****************************************************************************
 * Name: nxrmutex_is_hold
 *
 * Description:
 *   This function check whether the caller hold the recursive mutex
 *   referenced by 'rmutex'.
 *
 * Parameters:
 *   rmutex - Recursive mutex descriptor.
 *
 * Return Value:
 *
 ****************************************************************************/

static inline bool nxrmutex_is_hold(FAR rmutex_t *rmutex)
{
  return nxmutex_is_hold(&rmutex->mutex);
}

/****************************************************************************
 * Name: nxrmutex_is_locked
 *
 * Description:
 *   This function get the lock state the recursive mutex
 *   referenced by 'rmutex'.
 *
 * Parameters:
 *   rmutex - Recursive mutex descriptor.
 *
 * Return Value:
 *
 ****************************************************************************/

static inline bool nxrmutex_is_locked(FAR rmutex_t *rmutex)
{
  return nxmutex_is_locked(&rmutex->mutex);
}

/****************************************************************************
 * Name: nrxmutex_lock
 *
 * Description:
 *   This function attempts to lock the recursive mutex referenced by
 *   'rmutex'.The recursive mutex can be locked multiple times in the same
 *   thread.
 *
 * Parameters:
 *   rmutex - Recursive mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 ****************************************************************************/

static inline int nxrmutex_lock(FAR rmutex_t *rmutex)
{
  int ret = OK;

  if (!nxrmutex_is_hold(rmutex))
    {
      ret = nxmutex_lock(&rmutex->mutex);
    }

  if (ret >= 0)
    {
      DEBUGASSERT(rmutex->count < UINT_MAX);
      ++rmutex->count;
    }

  return ret;
}

/****************************************************************************
 * Name: nxrmutex_trylock
 *
 * Description:
 *   This function locks the recursive mutex if the recursive mutex is
 *   currently not locked or the same thread call.
 *   If the recursive mutex is locked and other thread call it,
 *   the call returns without blocking.
 *
 * Parameters:
 *   rmutex - Recursive mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 *     -EINVAL - Invalid attempt to lock the recursive mutex
 *     -EAGAIN - The recursive mutex is not available.
 *
 ****************************************************************************/

static inline int nxrmutex_trylock(FAR rmutex_t *rmutex)
{
  int ret = OK;

  if (!nxrmutex_is_hold(rmutex))
    {
      ret = nxmutex_trylock(&rmutex->mutex);
    }

  if (ret >= 0)
    {
      DEBUGASSERT(rmutex->count < UINT_MAX);
      ++rmutex->count;
    }

  return ret;
}

/****************************************************************************
 * Name: nxrmutex_timedlock
 *
 * Description:
 *   This function attempts to lock the mutex .  If the mutex value
 *   is (<=) zero,then the calling task will not return until it
 *   successfully acquires the lock or timed out
 *
 * Input Parameters:
 *   rmutex  - Rmutex object
 *   timeout - The time when mutex lock timed out
 *
 * Returned Value:
 *   OK        The mutex successfully acquires
 *   EINVAL    The mutex argument does not refer to a valid mutex.  Or the
 *             thread would have blocked, and the abstime parameter specified
 *             a nanoseconds field value less than zero or greater than or
 *             equal to 1000 million.
 *   ETIMEDOUT The mutex could not be locked before the specified timeout
 *             expired.
 *   EDEADLK   A deadlock condition was detected.
 *   ECANCELED May be returned if the thread is canceled while waiting.
 *
 ****************************************************************************/

static inline int nxrmutex_timedlock(FAR rmutex_t *rmutex,
                                     unsigned int timeout)
{
  int ret = OK;

  if (!nxrmutex_is_hold(rmutex))
    {
      ret = nxmutex_timedlock(&rmutex->mutex, timeout);
    }

  if (ret >= 0)
    {
      DEBUGASSERT(rmutex->count < UINT_MAX);
      ++rmutex->count;
    }

  return ret;
}

/****************************************************************************
 * Name: nxrmutex_unlock
 *
 * Description:
 *   This function attempts to unlock the recursive mutex
 *   referenced by 'rmutex'.
 *
 * Parameters:
 *   rmutex - Recursive mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 * Assumptions:
 *   This function may be called from an interrupt handler.
 *
 ****************************************************************************/

static inline int nxrmutex_unlock(FAR rmutex_t *rmutex)
{
  int ret = OK;

  DEBUGASSERT(rmutex->count > 0);

  if (--rmutex->count == 0)
    {
      ret = nxmutex_unlock(&rmutex->mutex);
      if (ret < 0)
        {
          ++rmutex->count;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: nxrmutex_reset
 *
 * Description:
 *   This function reset lock state.
 *
 * Parameters:
 *   rmutex - rmutex descriptor.
 *
 * Return Value:
 *
 ****************************************************************************/

static inline int nxrmutex_reset(FAR rmutex_t *rmutex)
{
  int ret;

  ret = nxmutex_reset(&rmutex->mutex);
  if (ret >= 0)
    {
      rmutex->count = 0;
    }

  return ret;
}

/****************************************************************************
 * Name: nrxmutex_breaklock
 *
 * Description:
 *   This function attempts to break the recursive mutex
 *
 * Parameters:
 *   rmutex - Recursive mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 ****************************************************************************/

static inline int nxrmutex_breaklock(FAR rmutex_t *rmutex,
                                     FAR unsigned int *count)
{
  int ret = OK;

  *count = 0;
  if (nxrmutex_is_hold(rmutex))
    {
      *count = rmutex->count;
      rmutex->count = 0;
      ret = nxmutex_unlock(&rmutex->mutex);
      if (ret < 0)
        {
          rmutex->count = *count;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: nxrmutex_restorelock
 *
 * Description:
 *   This function attempts to restore the recursive mutex.
 *
 * Parameters:
 *   rmutex - Recursive mutex descriptor.
 *
 * Return Value:
 *   This is an internal OS interface and should not be used by applications.
 *   It follows the NuttX internal error return policy:  Zero (OK) is
 *   returned on success.  A negated errno value is returned on failure.
 *   Possible returned errors:
 *
 ****************************************************************************/

static inline int nxrmutex_restorelock(FAR rmutex_t *rmutex,
                                       unsigned int count)
{
  int ret = OK;

  if (count != 0)
    {
      ret = nxmutex_lock(&rmutex->mutex);
      if (ret >= 0)
        {
          rmutex->count = count;
        }
    }

  return ret;
}

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __INCLUDE_NUTTX_MUTEX_H */
