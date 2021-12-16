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

#include "non_copyable_movable.h"
#include "poller.h"

namespace taotu {

class Eventer;

class EventManager : NonCopyableMovable {
 public:
  EventManager();
  ~EventManager();
  void RemoveEventer(Eventer *eventer);
  void UpdateEventer(Eventer *eventer);

 private:
  std::unique_ptr<Poller> poller;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_EVENT_MANAGER_H_
