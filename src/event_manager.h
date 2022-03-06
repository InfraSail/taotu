/**
 * @file event_manager.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_EVENT_MANAGER_H_
#define TAOTU_SRC_EVENT_MANAGER_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "connecting.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "poller.h"
#include "spin_lock.h"
#include "time_point.h"
#include "timer.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class EventManager : NonCopyableMovable {
 public:
  EventManager();
  ~EventManager();

  void Loop();
  void Work();

  // Insert a new connection into current I/O thread
  Connecting* InsertNewConnection(int socket_fd,
                                  const NetAddress& local_address,
                                  const NetAddress& peer_address);

  Poller* GetPoller() { return &poller_; }

  // For the Balancer to pick a EventManager with lowest load
  uint32_t GetEventerAmount() const {
    LockGuard lock_guard(connection_map_mutex_lock_);
    return connection_map_.size();
  }

  void RunAt(TimePoint time_point, Timer::TimeCallback TimeTask);
  void RunAfter(int64_t delay_microseconds, Timer::TimeCallback TimeTask);
  void RunEveryUntil(
      int64_t interval_microseconds, Timer::TimeCallback TimeTask,
      TimePoint start_time_point = TimePoint::FNow(),
      std::function<bool()> IsContinue = std::function<bool()>{});

  void DeleteConnection(int fd);

 private:
  typedef std::unordered_map<int, Connecting*> ConnectionMap;
  typedef std::vector<int> Fds;

  void DoWithActiveTasks(TimePoint return_time);
  void DoExpiredTimeTasks(TimePoint return_time);
  void DestroyClosedConnections();

  Poller poller_;
  ConnectionMap connection_map_;
  std::thread thread_;
  Timer timer_;

  mutable MutexLock connection_map_mutex_lock_;

  bool should_quit_;

  Poller::EventerList active_events_;
  Fds closed_fds_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_EVENT_MANAGER_H_
