/**
 * @file thread_pool.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-02-13
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "thread_pool.h"

using namespace taotu;

ThreadPool::ThreadPool() {}
ThreadPool::~ThreadPool() {
  for (auto& one_thread : threads_) {
    if (one_thread->joinable()) {
      one_thread->join();
    }
  }
}
