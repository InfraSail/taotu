#include <gtest/gtest.h>
#include <stdint.h>
#include <stdio.h>

#include <thread>

#include "../src/spin_lock.h"

TEST(LockTest, SpinLockTest) {
  taotu::MutexLock mutex_lock;
  uint64_t count = 0;
  auto f = [&]() {
    for (uint64_t i = 0; i < 100000000; ++i) {
      taotu::LockGuard lock_guard(mutex_lock);
      ++count;
    }
  };
  std::thread t1(f);
  std::thread t2(f);
  std::thread t3(f);
  if (t1.joinable()) {
    t1.join();
  }
  if (t2.joinable()) {
    t2.join();
  }
  if (t3.joinable()) {
    t3.join();
  }
  ASSERT_EQ(count, 300000000);
}
