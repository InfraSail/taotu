/**
 * @file event_manager.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "EventManager" (single "Reactor") which is the
 * manager of an event loop in single I/O thread.
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
#include <unordered_set>

#include "connecting.h"
#include "eventer.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "poller.h"
#include "spin_lock.h"
#include "time_point.h"
#include "timer.h"

namespace taotu {

/**
 * @brief "EventManager" manages the events (including I/O tasks and time tasks)
 * in a loop which uses I/O multiplexing and a time task queue. It is a
 * "Reactor".
 *
 */
class EventManager : NonCopyableMovable {
 public:
  EventManager();
  ~EventManager();

  // Run the loop in a new thread
  void Loop();
  // Run the loop in this thread
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

  // Register a time task which should be done in a future time point
  void RunAt(TimePoint time_point, Timer::TimeCallback TimeTask);
  // Register a time task which should be done after a certain duration
  void RunAfter(int64_t delay_microseconds, Timer::TimeCallback TimeTask);
  // Register a time task which should be done at certain intervals
  void RunEveryUntil(
      int64_t interval_microseconds, Timer::TimeCallback TimeTask,
      TimePoint start_time_point = TimePoint::FNow(),
      std::function<bool()> IsContinue = std::function<bool()>{});

  // Register a time task which should be done as soon as possible
  void RunSoon(Timer::TimeCallback TimeTask);

  // Delete the specific connection of this loop
  void DeleteConnection(int fd);

  // Wake up this I/O thread
  void WakeUp();

  // Quit this event loop (if using it in another thread, EventManager::WakeUp()
  // should be called after that)
  void Quit();

 private:
  typedef std::unordered_map<int, Connecting*> ConnectionMap;
  typedef std::unordered_set<int> Fds;

  // Handle I/O events
  void DoWithActiveTasks(TimePoint return_time);
  // Do time tasks
  void DoExpiredTimeTasks(TimePoint return_time);
  // Destroy connections which should be destroyed
  void DestroyClosedConnections();

  // I/O multiplexing manager
  Poller poller_;

  // All connnections in this loop (also within this thread) (Mapping: file
  // descriptor -> conncetion class pointer)
  ConnectionMap connection_map_;

  std::thread thread_;

  Timer timer_;

  // Spin lock protecting the connection map of this loop (also of this thread)
  mutable MutexLock connection_map_mutex_lock_;

  // The flag for deciding whether the event loop should quit
  bool should_quit_;

  // List for active events returned from the I/O multiplexing waiting each loop
  Poller::EventerList active_events_;

  // Set of file discriptors of connections which should be destroyed
  Fds closed_fds_;

  // Spin lock protecting the set of file discriptors of connections which
  // should be destroyed
  mutable MutexLock closed_fds_lock_;

  // To wake up this I/O thread
  Eventer wake_up_eventer_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_EVENT_MANAGER_H_
