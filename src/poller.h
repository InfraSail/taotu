/**
 * @file poller.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief io_uring-based I/O multiplexing wrapper.
 * @date 2024-xx-xx
 *
 * Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_POLLER_H_
#define TAOTU_SRC_POLLER_H_

#include <liburing.h>
#include <sys/uio.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "non_copyable_movable.h"
#include "time_point.h"

#ifndef __linux__
#error "io_uring backend requires Linux."
#endif

namespace taotu {

class Eventer;

class Poller : NonCopyableMovable {
 public:
  typedef std::vector<Eventer*> EventerList;

  enum class OpType { kPoll, kRead, kWrite, kAccept, kTimeout, kNone };

  struct IoUringOp;
  typedef void (*CompletionFn)(struct io_uring_cqe* cqe, IoUringOp* op);
  typedef void (*ContextDeleter)(void* context);

  struct IoUringOp {
    OpType type{OpType::kNone};
    Eventer* eventer{nullptr};
    void* context{nullptr};
    int fd{-1};
    CompletionFn completion{nullptr};
    uint64_t key{0};
    ContextDeleter context_deleter{nullptr};
  };

  Poller();
  ~Poller();

  // Poll the completion queue, return current time, and fill active Eventers.
  TimePoint Poll(int timeout, EventerList* active_eventers);

  void AddEventer(Eventer* eventer);
  void ModifyEventer(Eventer* eventer);
  void RemoveEventer(Eventer* eventer);

  // Submit I/O operations directly.
  uint64_t SubmitRead(Eventer* eventer, struct iovec* iov, int iovcnt,
                      CompletionFn completion = nullptr, void* ctx = nullptr,
                      uint64_t key = 0,
                      ContextDeleter context_deleter = nullptr);
  uint64_t SubmitReadMultishot(Eventer* eventer, int buf_group,
                               CompletionFn completion = nullptr,
                               void* ctx = nullptr, uint64_t key = 0,
                               ContextDeleter context_deleter = nullptr);
  uint64_t SubmitWrite(Eventer* eventer, struct iovec* iov, int iovcnt,
                       CompletionFn completion = nullptr, void* ctx = nullptr,
                       uint64_t key = 0,
                       ContextDeleter context_deleter = nullptr);
  uint64_t SubmitAccept(int fd, struct sockaddr* addr, socklen_t* addrlen,
                        void* ctx, CompletionFn completion = nullptr,
                        uint64_t key = 0, bool multishot = false,
                        ContextDeleter context_deleter = nullptr);

  void CancelOp(uint64_t user_data_key);

  // Limit CQE handling per poll to avoid starving timers.
  void SetCqeBatchLimit(size_t limit) { cqe_batch_limit_ = limit; }
  void SetCqeTimeBudgetUs(int64_t budget_us) {
    cqe_time_budget_us_ = budget_us;
  }

  bool UseSqpoll() const { return use_sqpoll_; }
  bool UseMultishotAccept() const { return use_multishot_accept_; }
  bool BuffersRegistered() const { return buffers_registered_; }
  size_t BufferCount() const { return kBufCount; }
  char* GetBuffer(uint16_t id);
  static constexpr int kBufferGroupId = 1;
  static constexpr size_t kBufSize = 64 * 1024;
  static constexpr size_t kBufCount = 64;

 private:
  struct EventerState {
    uint32_t mask{0};   // Event mask of interest (POLLIN/POLLOUT).
    bool armed{false};  // Whether a poll request is already pending.
    uint64_t poll_key{0};
  };

  uint64_t NormalizeKey(uint64_t key);
  std::unique_ptr<IoUringOp> LookupOp(uint64_t key);
  void CleanupOpContext(IoUringOp* op);

  void SubmitPoll(Eventer* eventer);
  void CancelPoll(Eventer* eventer);
  void HandleCqe(struct io_uring_cqe* cqe, EventerList* active_eventers);
  void SubmitPending();
  void RegisterBuffers();
  void UnregisterBuffers();
  void ReleaseBufferFromCqe(struct io_uring_cqe* cqe);

  struct io_uring ring_;
  std::unordered_map<Eventer*, EventerState> states_;
  std::unordered_map<uint64_t, std::unique_ptr<IoUringOp>> ops_;
  std::atomic_uint64_t next_key_{1};
  mutable std::mutex ops_mutex_;
  bool use_sqpoll_{false};
  bool use_multishot_accept_{true};
  bool buffers_registered_{false};
  std::array<char[kBufSize], kBufCount> buffers_{};
  size_t cqe_batch_limit_{1024};
  int64_t cqe_time_budget_us_{1000};
};

}  // namespace taotu

#endif  // TAOTU_SRC_POLLER_H_
