/**
 * @file thread_pool.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "ThreadPool" which is the caculation thread pool.
 * @date 2022-02-13
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_SRC_THREAD_POOL_H_
#define TAOTU_SRC_THREAD_POOL_H_

#include <stdlib.h>

#include <array>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

#include "logger.h"
#include "non_copyable_movable.h"

namespace taotu {

/**
 * @brief "ThreadPool" use high concurrent design to make the procedures of
 * executing and adding tasks fast.
 *
 */
class ThreadPool : NonCopyableMovable {
 public:
  explicit ThreadPool(size_t thread_amount = 4);
  ~ThreadPool();

  // Order the thread pool to execute a function and reture a "std::future"
  // object for retrieving the return value
  template <class Function>
  auto AddTask(std::function<Function>&& task)
      -> std::future<typename std::invoke_result<Function>::type> {
    if (should_stop_) {
      LOG(logger::kWarn,
          "Fail to add a task into the calculation thread pool!");
      return std::nullopt;
    } else {
      auto task_future_pkg = std::make_shared<std::packaged_task<Function>>(
          std::forward<std::function<Function>>(task));
      auto result_future = task_future_pkg->get_future();
      std::lock_guard<std::mutex> lock(que_csm_mutex_);
      task_queues_[que_pdt_idx_].emplace(
          [task_future_pkg]() { (*task_future_pkg)(); });
      pdt_csm_cond_var_
          .notify_one();  // Awake one thread to consume (after this producing)
      return result_future;
    }
  }

 private:
  std::vector<std::unique_ptr<std::thread>> threads_;

  // 2 task queues (one is for producers and the other is for consumers)
  std::array<std::queue<std::function<void()>>, 2> task_queues_;

  // Index of the task queue for producers
  size_t que_pdt_idx_;

  // Mutex lock protecting the 2 task queues
  std::mutex pdt_csm_mutex_;

  // Condition Variable for blocking the current leisure thread (and release the
  // lock temporarily) and awaking a ready thread
  std::condition_variable pdt_csm_cond_var_;

  // Mutex lock protecting the task queue for consumers
  std::mutex que_csm_mutex_;

  bool should_stop_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_THREAD_POOL_H_
