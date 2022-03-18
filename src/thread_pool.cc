/**
 * @file thread_pool.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "ThreadPool" which is the caculation thread
 * pool.
 * @date 2022-02-13
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "thread_pool.h"

#include <mutex>
#include <thread>
#include <utility>

#include "logger.h"

using namespace taotu;

ThreadPool::ThreadPool(size_t thread_amount)
    : que_pdt_idx_(0), should_stop_(false) {
  for (size_t i = 0; i < thread_amount; ++i) {
    threads_.emplace_back(std::make_unique<std::thread>([this]() {
      while (true) {
        std::function<void()> CurTask;
        {
          std::unique_lock<std::mutex> lock(this->pdt_csm_mutex_);
          this->pdt_csm_cond_var_.wait(lock, [this]() {
            return this->should_stop_ ||
                   !(this->task_queues_[this->que_pdt_idx_].empty()) ||
                   !(this->task_queues_[!(this->que_pdt_idx_)].empty());
          });  // If there is no task or the thread pool should stop, blocking
               // the current thread and release the lock temporarily
          if (this->should_stop_ &&
              this->task_queues_[this->que_pdt_idx_].empty() &&
              this->task_queues_[!(this->que_pdt_idx_)]
                  .empty()) {  // If there is no task, the thread pool should
                               // stop
            return;
          }
          size_t que_csm_idx = !(this->que_pdt_idx_);
          if (this->task_queues_[que_csm_idx]
                  .empty()) {  // If there is no task for cunsuming, make the
                               // index of the task queue for producers as the
                               // index of the task queue for consumers
            std::lock_guard<std::mutex> csm_lock(this->que_csm_mutex_);
            this->que_pdt_idx_ = que_csm_idx;
          }
          que_csm_idx =
              !(this->que_pdt_idx_);  // Make the index of the task queue for
                                      // consumers as the previous index of the
                                      // task queue for producers

          // Retrieve a task from the task queue for consumers
          CurTask = std::move(this->task_queues_[que_csm_idx].front());
          this->task_queues_[que_csm_idx].pop();
        }
        if (CurTask) {  // Execute the task in the current thread
          CurTask();
        }
      }
    }));
  }
}
ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> lock(pdt_csm_mutex_);
    should_stop_ = true;
    pdt_csm_cond_var_.notify_all();
  }
  for (auto& one_thread : threads_) {
    if (one_thread->joinable()) {
      one_thread->join();
    }
  }
}

void ThreadPool::AddTask(std::function<void()> task) {
  if (should_stop_) {
    LOG(logger::kWarn, "Fail to add a task into the calculation thread pool!");
  } else {
    std::lock_guard<std::mutex> lock(que_csm_mutex_);
    task_queues_[que_pdt_idx_].emplace(std::move(task));
    pdt_csm_cond_var_
        .notify_one();  // Awake one thread to consume (after this producing)
  }
}
