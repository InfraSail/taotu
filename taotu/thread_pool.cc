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

#include <mutex>
#include <thread>
#include <utility>

#include "logger.h"

using namespace taotu;

ThreadPool::ThreadPool(int thread_amount)
    : cur_que_idx_(0), should_stop_(false) {
  for (int i = 0; i < thread_amount; ++i) {
    threads_.emplace_back(std::make_unique<std::thread>([this]() {
      while (true) {
        std::function<void()> CurTask;
        {
          std::unique_lock<std::mutex> lock(this->pdt_csm_mutex_);
          this->pdt_csm_cond_var_.wait(lock, [this]() {
            return this->should_stop_ ||
                   !(this->task_queues_[this->cur_que_idx_].empty()) ||
                   !(this->task_queues_[!(this->cur_que_idx_)].empty());
          });
          if (this->should_stop_ &&
              this->task_queues_[this->cur_que_idx_].empty() &&
              this->task_queues_[!(this->cur_que_idx_)].empty()) {
            return;
          }
          size_t tmp_que_idx = !(this->cur_que_idx_);
          if (this->task_queues_[tmp_que_idx].empty()) {
            std::lock_guard<std::mutex> exc_lock(this->que_exc_mutex_);
            this->cur_que_idx_ = tmp_que_idx;
          }
          tmp_que_idx = !(this->cur_que_idx_);
          CurTask = std::move(this->task_queues_[tmp_que_idx].front());
          this->task_queues_[tmp_que_idx].pop();
        }
        if (CurTask) {
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
    LOG(logger::kDebug,
        "Fail to add a task into the calculation thread pool!!!");
  } else {
    std::lock_guard<std::mutex> lock(que_exc_mutex_);
    task_queues_[cur_que_idx_].emplace(std::move(task));
    pdt_csm_cond_var_.notify_one();
  }
}
