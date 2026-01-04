/**
 * @file poller.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief io_uring-based I/O multiplexing implementation.
 * @date 2024-xx-xx
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "poller.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cstdint>
#include <string>

#include "eventer.h"
#include "logger.h"

namespace taotu {
namespace {
constexpr uint32_t kDefaultEntries = 32768;
constexpr uint32_t kMinEntries = 1024;

uint32_t GetIoUringEntries() {
  const char* env = ::getenv("TAOTU_IORING_ENTRIES");
  if (!env || *env == '\0') {
    return kDefaultEntries;
  }
  char* end = nullptr;
  uint64_t val = ::strtoull(env, &end, 10);
  if (end == env || val == 0) {
    return kDefaultEntries;
  }
  if (val > kDefaultEntries) {
    return kDefaultEntries;
  }
  if (val < kMinEntries) {
    return kMinEntries;
  }
  return static_cast<uint32_t>(val);
}
}  // namespace

Poller::Poller() {
  ::memset(static_cast<void*>(&ring_), 0, sizeof(ring_));
  struct io_uring_params params {};
  params.flags = IORING_SETUP_SQPOLL;
  uint32_t entries = GetIoUringEntries();
  int ret = -ENOMEM;
  while (entries >= kMinEntries) {
    ret = ::io_uring_queue_init_params(entries, &ring_, &params);
    if (ret == -ENOMEM && entries > kMinEntries) {
      entries /= 2;
      continue;
    }
    break;
  }
  if (ret == -EPERM || ret == -EINVAL) {
    LOG_WARN("io_uring SQPOLL unavailable, fallback to default: %s",
             ::strerror(-ret));
    ::memset(static_cast<void*>(&ring_), 0, sizeof(ring_));
    ::memset(static_cast<void*>(&params), 0, sizeof(params));
    entries = GetIoUringEntries();
    ret = -ENOMEM;
    while (entries >= kMinEntries) {
      ret = ::io_uring_queue_init_params(entries, &ring_, &params);
      if (ret == -ENOMEM && entries > kMinEntries) {
        entries /= 2;
        continue;
      }
      break;
    }
  } else {
    use_sqpoll_ = true;
  }
  if (ret < 0) {
    LOG_ERROR("io_uring_queue_init failed: %s", ::strerror(-ret));
    ::exit(EXIT_FAILURE);
  }
  if (use_sqpoll_) {
    LOG_DEBUG("io_uring initialized with SQPOLL.");
  }
  struct io_uring_probe* probe = ::io_uring_get_probe_ring(&ring_);
  if (probe) {
    if (!::io_uring_opcode_supported(probe, IORING_OP_ACCEPT)) {
      use_multishot_accept_ = false;
      LOG_WARN("io_uring_accept not supported; multishot accept disabled.");
    }
    ::io_uring_free_probe(probe);
  }
  RegisterBuffers();
}

Poller::~Poller() {
  // Clean up user-space ops still in the queue to avoid leaks on early exit.
  LOG_DEBUG("Destroying Poller, pending ops: %zu", ops_.size());
  {
    std::lock_guard<std::mutex> lock(ops_mutex_);
    for (auto& item : ops_) {
      CleanupOpContext(item.second.get());
    }
    ops_.clear();  // unique_ptr releases automatically.
  }
  UnregisterBuffers();
  ::io_uring_queue_exit(&ring_);
}

uint64_t Poller::NormalizeKey(uint64_t key) {
  if (key == 0) {
    key = next_key_.fetch_add(1, std::memory_order_relaxed);
    if (key == 0) {  // overflow protection: skip 0
      key = next_key_.fetch_add(1, std::memory_order_relaxed);
    }
  }
  return key;
}

std::unique_ptr<Poller::IoUringOp> Poller::LookupOp(uint64_t key) {
  if (key == 0) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(ops_mutex_);
  auto it = ops_.find(key);
  if (it == ops_.end()) {
    return nullptr;
  }
  auto op = std::move(it->second);
  ops_.erase(it);
  return op;
}

void Poller::AddEventer(Eventer* eventer) {
  states_[eventer] = EventerState{eventer->Events(), false};
  SubmitPoll(eventer);
  SubmitPending();
}

void Poller::ModifyEventer(Eventer* eventer) {
  auto itr = states_.find(eventer);
  if (itr == states_.end()) {
    AddEventer(eventer);
    return;
  }
  itr->second.mask = eventer->Events();
  if (itr->second.mask == 0) {
    CancelPoll(eventer);
  } else {
    SubmitPoll(eventer);
  }
  SubmitPending();
}

void Poller::RemoveEventer(Eventer* eventer) {
  CancelPoll(eventer);
  states_.erase(eventer);
}

uint64_t Poller::SubmitRead(Eventer* eventer, struct iovec* iov, int iovcnt,
                            CompletionFn completion, void* ctx, uint64_t key,
                            ContextDeleter context_deleter) {
  key = NormalizeKey(key);
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_ERROR("io_uring_get_sqe failed when submit read fd(%d)", eventer->Fd());
    return 0;
  }
  auto op = std::make_unique<IoUringOp>(IoUringOp{OpType::kRead, eventer, ctx,
                                                  eventer->Fd(), completion,
                                                  key, context_deleter});
  {
    std::lock_guard<std::mutex> lock(ops_mutex_);
    ops_[key] = std::move(op);
  }
  ::io_uring_prep_readv(sqe, eventer->Fd(), iov, iovcnt, 0);
  ::io_uring_sqe_set_data64(sqe, key);
  SubmitPending();
  return key;
}

uint64_t Poller::SubmitReadMultishot(Eventer* eventer, int buf_group,
                                     CompletionFn completion, void* ctx,
                                     uint64_t key,
                                     ContextDeleter context_deleter) {
#ifdef IORING_OP_RECV_MULTISHOT
  key = NormalizeKey(key);
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_ERROR("io_uring_get_sqe failed when submit recv-multishot fd(%d)",
              eventer->Fd());
    return 0;
  }
  auto op = std::make_unique<IoUringOp>(IoUringOp{OpType::kRead, eventer, ctx,
                                                  eventer->Fd(), completion,
                                                  key, context_deleter});
  {
    std::lock_guard<std::mutex> lock(ops_mutex_);
    ops_[key] = std::move(op);
  }
  ::io_uring_prep_recv_multishot(sqe, eventer->Fd(), nullptr, 0, 0);
  ::io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT);
  sqe->buf_group = static_cast<__u16>(buf_group);
  ::io_uring_sqe_set_data64(sqe, key);
  SubmitPending();
  return key;
#else
  (void)eventer;
  (void)buf_group;
  (void)completion;
  (void)ctx;
  (void)key;
  LOG_WARN("recv-multishot not supported, skip submit.");
  return 0;
#endif
}

uint64_t Poller::SubmitWrite(Eventer* eventer, struct iovec* iov, int iovcnt,
                             CompletionFn completion, void* ctx, uint64_t key,
                             ContextDeleter context_deleter) {
  key = NormalizeKey(key);
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_ERROR("io_uring_get_sqe failed when submit write fd(%d)",
              eventer->Fd());
    return 0;
  }
  auto op = std::make_unique<IoUringOp>(IoUringOp{OpType::kWrite, eventer, ctx,
                                                  eventer->Fd(), completion,
                                                  key, context_deleter});
  {
    std::lock_guard<std::mutex> lock(ops_mutex_);
    ops_[key] = std::move(op);
  }
  ::io_uring_prep_writev(sqe, eventer->Fd(), iov, iovcnt, 0);
  ::io_uring_sqe_set_data64(sqe, key);
  SubmitPending();
  return key;
}

uint64_t Poller::SubmitAccept(int fd, struct sockaddr* addr, socklen_t* addrlen,
                              void* ctx, CompletionFn completion, uint64_t key,
                              bool multishot, ContextDeleter context_deleter) {
  key = NormalizeKey(key);
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_ERROR("io_uring_get_sqe failed when submit accept fd(%d)", fd);
    return 0;
  }
  auto op = std::make_unique<IoUringOp>(IoUringOp{
      OpType::kAccept, nullptr, ctx, fd, completion, key, context_deleter});
  {
    std::lock_guard<std::mutex> lock(ops_mutex_);
    ops_[key] = std::move(op);
  }
  if (multishot && use_multishot_accept_) {
#ifdef IORING_ACCEPT_MULTISHOT
    ::io_uring_prep_multishot_accept(sqe, fd, addr, addrlen,
                                     SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    ::io_uring_prep_accept(sqe, fd, addr, addrlen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  } else {
    ::io_uring_prep_accept(sqe, fd, addr, addrlen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
  }
  ::io_uring_sqe_set_data64(sqe, key);
  SubmitPending();
  return key;
}

void Poller::CancelOp(uint64_t user_data_key) {
  if (user_data_key == 0) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(ops_mutex_);
    auto it = ops_.find(user_data_key);
    if (it != ops_.end()) {
      // Mark canceled: keep op until its CQE arrives, so the kernel won't
      // touch freed context/iov memory.
      it->second->eventer = nullptr;
      it->second->completion = nullptr;
    }
  }
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_ERROR("io_uring_get_sqe failed when cancel op");
    return;
  }
  ::io_uring_prep_cancel64(sqe, user_data_key, 0);
  ::io_uring_sqe_set_data64(sqe, 0);  // Cancellation CQE needs no handling.
  SubmitPending();
}
TimePoint Poller::Poll(int timeout, EventerList* active_eventers) {
  struct __kernel_timespec ts {};
  struct __kernel_timespec* tsp = nullptr;
  if (timeout >= 0) {
    ts.tv_sec = timeout / 1000;
    ts.tv_nsec = (timeout % 1000) * 1000000;
    tsp = &ts;
  }

  struct io_uring_cqe* cqe = nullptr;
  int ret = ::io_uring_wait_cqe_timeout(&ring_, &cqe, tsp);
  if (ret == -ETIME) {
    SubmitPending();
    return TimePoint{};
  }
  if (ret < 0) {
    LOG_ERROR("io_uring_wait_cqe_timeout failed: %s", ::strerror(-ret));
    SubmitPending();
    return TimePoint{};
  }

  const int64_t start_us = TimePoint::FNow();
  HandleCqe(cqe, active_eventers);
  ::io_uring_cqe_seen(&ring_, cqe);

  // Continue draining all completed CQEs.
  const size_t limit = cqe_batch_limit_;
  const int64_t budget_us = cqe_time_budget_us_;
  size_t handled = 1;
  while (limit == 0 || handled < limit) {
    if (budget_us > 0 && (TimePoint::FNow() - start_us) >= budget_us) {
      break;
    }
    ret = ::io_uring_peek_cqe(&ring_, &cqe);
    if (ret == -EAGAIN) {
      break;
    } else if (ret < 0) {
      LOG_ERROR("io_uring_peek_cqe failed: %s", ::strerror(-ret));
      break;
    }
    HandleCqe(cqe, active_eventers);
    ::io_uring_cqe_seen(&ring_, cqe);
    ++handled;
  }

  SubmitPending();
  return TimePoint{};
}

void Poller::HandleCqe(struct io_uring_cqe* cqe, EventerList* active_eventers) {
  uint64_t key = cqe->user_data;
  if (key == 0) {
    ReleaseBufferFromCqe(cqe);
    return;  // cancellation or ignored CQE
  }
  auto op = LookupOp(key);
  if (!op) {
    ReleaseBufferFromCqe(cqe);
    return;
  }
  auto* op_ptr = op.get();
  LOG_DEBUG("CQE type(%d) res(%d) user_data(%llu) completion(%p)",
            static_cast<int>(op_ptr->type), cqe->res,
            static_cast<unsigned long long>(key),
            reinterpret_cast<void*>(op_ptr->completion));
  bool keep_op = (cqe->flags & IORING_CQE_F_MORE) != 0;
  if (op_ptr->completion) {
    LOG_DEBUG("Call completion for type(%d)", static_cast<int>(op_ptr->type));
    op_ptr->completion(cqe, op_ptr);
    ReleaseBufferFromCqe(cqe);
    if (keep_op && op && op->context != nullptr) {
      std::lock_guard<std::mutex> lock(ops_mutex_);
      ops_[key] = std::move(op);  // keep for the next CQE
    } else {
      op.reset();
    }
    return;
  }
  if (op_ptr->context != nullptr) {
    CleanupOpContext(op_ptr);
  }
  switch (op_ptr->type) {
    case OpType::kPoll: {
      auto* eventer = op_ptr->eventer;
      if (eventer == nullptr) {
        break;  // Eventer was removed, ignore this CQE
      }
      auto itr = states_.find(eventer);
      if (itr != states_.end()) {
        itr->second.armed = false;
        if (cqe->res >= 0) {
          eventer->ReceiveEvents(static_cast<uint32_t>(cqe->res));
          active_eventers->push_back(eventer);
        } else {
          LOG_ERROR("io_uring poll on fd(%d) failed: %s", eventer->Fd(),
                    ::strerror(-cqe->res));
        }
        SubmitPoll(eventer);
      }
      // If not in states_, eventer was removed - ignore this late CQE
      break;
    }
    case OpType::kRead: {
      auto* eventer = op_ptr->eventer;
      if (eventer == nullptr || states_.find(eventer) == states_.end()) {
        ReleaseBufferFromCqe(cqe);
        break;  // Eventer was removed, ignore
      }
      Eventer::ReadResult rr{.bytes = cqe->res,
                             .err = cqe->res < 0 ? -cqe->res : 0};
      ReleaseBufferFromCqe(cqe);
      eventer->OnReadDone(rr, TimePoint{});
      break;
    }
    case OpType::kWrite: {
      auto* eventer = op_ptr->eventer;
      if (eventer == nullptr || states_.find(eventer) == states_.end()) {
        break;  // Eventer was removed, ignore
      }
      Eventer::WriteResult wr{.bytes = cqe->res,
                              .err = cqe->res < 0 ? -cqe->res : 0};
      eventer->OnWriteDone(wr);
      break;
    }
    case OpType::kAccept: {
      auto* eventer = op_ptr->eventer;
      if (eventer) {
        eventer->OnAcceptDone(static_cast<int>(cqe->res), nullptr, 0);
      }
      break;
    }
    case OpType::kTimeout:
    case OpType::kNone:
      break;
  }
  op.reset();
}

void Poller::CleanupOpContext(IoUringOp* op) {
  if (!op || op->context == nullptr || op->context_deleter == nullptr) {
    return;
  }
  op->context_deleter(op->context);
  op->context = nullptr;
}

void Poller::SubmitPoll(Eventer* eventer) {
  auto itr = states_.find(eventer);
  if (itr == states_.end()) {
    return;
  }
  auto& state = itr->second;
  if (state.mask == 0 || state.armed) {
    return;
  }
  uint64_t key = NormalizeKey(0);
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_ERROR("io_uring_get_sqe failed when arming fd(%d)", eventer->Fd());
    return;
  }
  auto op = std::make_unique<IoUringOp>(
      IoUringOp{OpType::kPoll, eventer, nullptr, eventer->Fd(), nullptr, key});
  {
    std::lock_guard<std::mutex> lock(ops_mutex_);
    ops_[key] = std::move(op);
  }
  ::io_uring_prep_poll_add(sqe, eventer->Fd(),
                           static_cast<unsigned>(state.mask));
  ::io_uring_sqe_set_data64(sqe, key);
  state.armed = true;
  state.poll_key = key;
}

void Poller::CancelPoll(Eventer* eventer) {
  auto itr = states_.find(eventer);
  if (itr == states_.end() || !itr->second.armed) {
    return;
  }
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_ERROR("io_uring_get_sqe failed when canceling fd(%d)", eventer->Fd());
    return;
  }
  ::io_uring_prep_poll_remove(sqe, itr->second.poll_key);
  ::io_uring_sqe_set_data(sqe, nullptr);  // Ignore cancellation result.
  itr->second.armed = false;
  itr->second.poll_key = 0;
}

void Poller::SubmitPending() {
  int ret = ::io_uring_submit(&ring_);
  if (ret < 0) {
    LOG_ERROR("io_uring_submit failed: %s", ::strerror(-ret));
  }
}

void Poller::RegisterBuffers() {
#ifdef IORING_OP_RECV_MULTISHOT
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_WARN("io_uring_get_sqe failed when registering buffers, skip.");
    return;
  }
  struct io_uring_recvmsg_out out;  // dummy to silence potential warnings
  (void)out;
  ::io_uring_prep_provide_buffers(sqe, buffers_.data(), kBufSize, kBufCount,
                                  kBufferGroupId, 0);
  ::io_uring_sqe_set_data64(sqe, 0);  // ignored CQE
  int ret = ::io_uring_submit(&ring_);
  if (ret < 0) {
    LOG_WARN("io_uring_submit provide_buffers failed: %s", ::strerror(-ret));
    return;
  }
  struct io_uring_cqe* cqe = nullptr;
  ret = ::io_uring_wait_cqe(&ring_, &cqe);
  if (ret == 0 && cqe->res >= 0) {
    buffers_registered_ = true;
  } else {
    LOG_WARN("provide_buffers CQE failed: %s", ::strerror(-ret));
  }
  ::io_uring_cqe_seen(&ring_, cqe);
#endif
}

void Poller::UnregisterBuffers() {
#ifdef IORING_OP_RECV_MULTISHOT
  if (!buffers_registered_) {
    return;
  }
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    return;
  }
  ::io_uring_prep_remove_buffers(sqe, kBufCount, kBufferGroupId);
  ::io_uring_sqe_set_data64(sqe, 0);
  ::io_uring_submit(&ring_);
  buffers_registered_ = false;
#endif
}

void Poller::ReleaseBufferFromCqe(struct io_uring_cqe* cqe) {
#ifdef IORING_OP_RECV_MULTISHOT
  if (!buffers_registered_) {
    return;
  }
  if (!(cqe->flags & IORING_CQE_F_BUFFER)) {
    return;
  }
  uint16_t bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
  if (bid >= kBufCount) {
    LOG_WARN("buffer id out of range: %u", bid);
    return;
  }
  struct io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_);
  if (!sqe) {
    LOG_WARN("io_uring_get_sqe failed when release buffer");
    return;
  }
  ::io_uring_prep_provide_buffers(sqe, buffers_[bid], kBufSize, 1,
                                  kBufferGroupId, bid);
  ::io_uring_sqe_set_data64(sqe, 0);
#endif
}

char* Poller::GetBuffer(uint16_t id) {
  if (!buffers_registered_ || id >= kBufCount) {
    return nullptr;
  }
  return buffers_[id];
}

}  // namespace taotu
