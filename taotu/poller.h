/**
 * @file poller.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_POLLER_H_
#define TAOTU_TAOTU_POLLER_H_

#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

#include "non_copyable_movable.h"
#include "time_point.h"

namespace taotu {

class Eventer;

/**
 * @brief  // TODO:
 *
 */
class Poller : NonCopyableMovable {
 public:
  typedef std::vector<Eventer*> EventerList;

  Poller();
  ~Poller();

  void Poll(int timeout, EventerList* active_eventers);

  void AddEventer(Eventer* eventer);
  void ModifyEventer(Eventer* eventer);
  void RemoveEventer(Eventer* eventer);

 private:
  void GetActiveEventer(int event_amount, EventerList* active_eventers) const;

  bool IsPollFdEffective();

  // File descriptor of native "poll()" (poll, epoll or kqueue)
  int poll_fd_;

  typedef std::unordered_map<int, Eventer*> EventerMap;
  typedef std::vector<struct epoll_event> PollEventList;

  EventerMap eventers_;
  PollEventList poll_events_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_POLLER_H_
