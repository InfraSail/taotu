/**
 * @file poller.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Poller" which is the encapsulation of I/O
 * multiplexing.
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_POLLER_H_
#define TAOTU_SRC_POLLER_H_

#include <stdint.h>
#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

#include "non_copyable_movable.h"
#include "time_point.h"

namespace taotu {

class Eventer;

/**
 * @brief "Poller" waits for triggered events and transmits them to "Eventer"
 * (they will be handled by its member function "Loop()").
 *
 */
class Poller : NonCopyableMovable {
 public:
  typedef std::vector<Eventer*> EventerList;

  Poller();
  ~Poller();

  // Polling (calls the native poll() (poll, epoll or kqueue))
  TimePoint Poll(int timeout, EventerList* active_eventers);

  void AddEventer(Eventer* eventer);
  void ModifyEventer(Eventer* eventer);
  void RemoveEventer(Eventer* eventer);

 private:
  typedef std::vector<struct epoll_event> PollEventList;

  void GetActiveEventer(int event_amount, EventerList* active_eventers) const;

  bool IsPollFdEffective() const;

  // File descriptor of the native poll() (poll, epoll or kqueue)
  int poll_fd_;

  // The buffer for active event struct of the native poll() (poll, epoll or
  // kqueue)
  PollEventList poll_events_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_POLLER_H_
