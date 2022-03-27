/**
 * @file eventer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Eventer" which is "single event operator".
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_EVENTER_H_
#define TAOTU_SRC_EVENTER_H_

#include <sys/poll.h>

#include <functional>
#include <memory>
#include <utility>

#include "non_copyable_movable.h"
#include "poller.h"
#include "time_point.h"

namespace taotu {

class Poller;

/**
 * @brief "Eventer" is the encapsulation of "event" which is relavant to I/O
 * multiplexing and I/O callbacks.
 *
 */
class Eventer : NonCopyableMovable {
 public:
  typedef std::function<void()> NormalCallback;
  typedef std::function<void(TimePoint)> ReadCallback;

  Eventer(Poller* poller, int fd);
  ~Eventer();

  // Handle all events
  void Work(TimePoint tp);

  void RegisterReadCallback(ReadCallback cb) { ReadCallback_ = std::move(cb); }
  void RegisterWriteCallback(NormalCallback cb) {
    WriteCallback_ = std::move(cb);
  }
  void RegisterCloseCallback(NormalCallback cb) {
    CloseCallback_ = std::move(cb);
  }
  void RegisterErrorCallback(NormalCallback cb) {
    ErrorCallback_ = std::move(cb);
  }

  int Fd() const { return fd_; }
  uint32_t Events() const { return out_events_; }

  void ReceiveEvents(uint32_t in_events) { in_events_ = in_events; }

  bool HasNoEvent() const { return out_events_ == kNoEvent; }
  bool HasReadEvents() const { return out_events_ & kReadEvents; }
  bool HasWriteEvents() const { return out_events_ & kWriteEvents; }

  void EnableReadEvents();
  void DisableReadEvents();
  void EnableWriteEvents();
  void DisableWriteEvents();
  void DisableAllEvents();

  // Only used in Connector::RemoveAndReset() (prepare for EventerPtr::reset())
  void GetReadyDestroy() { is_handling_ = false; }

  void RemoveMyself();

 private:
  void UpdateEvents();

  // In <poll.h> or <sys/epoll.h>
  enum {
    kNoEvent = 0x0000,
    kReadEvents = POLLIN | POLLPRI,
    kWriteEvents = POLLOUT,
  };

  // Pointer to its master poller
  Poller* poller_;

  const int fd_;

  // For receiving
  uint32_t in_events_;

  // For setting
  uint32_t out_events_;

  bool is_handling_;

  // Callback function which will be called after each reading
  ReadCallback ReadCallback_;

  // Callback function which will be called after each real writing
  NormalCallback WriteCallback_;

  // Callback function which will be called when this TCP connection should be
  // closed
  NormalCallback CloseCallback_;

  // Callback function which will be called when error happens
  NormalCallback ErrorCallback_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_EVENTER_H_
