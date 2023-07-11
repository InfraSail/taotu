/**
 * @file spin_lock.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration and implementation of class "MutexLock" which is a mutex
 * lock and class "LockGuard" which is a life auto manager for mutex locks.
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_SPIN_LOCK_H_
#define TAOTU_SRC_SPIN_LOCK_H_

#include <atomic>

#include "non_copyable_movable.h"

namespace taotu {

/**
 * @brief "MutexLock" is a mutex lock which improves the performance of the
 * program when locking time is short by using lock-free programming.
 */
class MutexLock : NonCopyableMovable {
 public:
  MutexLock() : value_(true) {}
  ~MutexLock() { Unlock(); }

  void Lock() {
    bool exp = true;
    while (!value_.compare_exchange_weak(exp, false)) {
      exp = true;
    }
  }
  void Unlock() { value_.store(true); }

 private:
  std::atomic_bool value_;
};

/**
 * @brief "LockGuard" is a life auto manager for mutex locks by using RAII
 * mechanism.
 *
 */
class LockGuard : NonCopyableMovable {
 public:
  LockGuard(MutexLock& mutex_lock) : mutex_lock_(mutex_lock) {
    mutex_lock_.Lock();
  }
  ~LockGuard() { mutex_lock_.Unlock(); }

 private:
  MutexLock& mutex_lock_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_SPIN_LOCK_H_
