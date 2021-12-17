/**
 * @file event_manager.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_EVENT_MANAGER_H_
#define TAOTU_TAOTU_EVENT_MANAGER_H_

#include <memory>
#include <thread>
#include <vector>

#include "non_copyable_movable.h"
#include "poller.h"

namespace taotu {

class Eventer;

class EventManager : NonCopyableMovable {
 public:
  EventManager();
  ~EventManager();

  void Loop();

  void RemoveEventer(Eventer *eventer);
  void UpdateEventer(Eventer *eventer);

 private:
  std::unique_ptr<Poller> poller;
  std::vector<Eventer> eventers_;
  std::unique_ptr<std::thread> thread_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_EVENT_MANAGER_H_