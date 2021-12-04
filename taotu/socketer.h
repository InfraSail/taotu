/**
 * @file socketer.h
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief  // TODO:
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_SOCKETER_H_
#define TAOTU_TAOTU_SOCKETER_H_

#include <poll.h>

#include <functional>
#include <memory>

#include "eventer.h"
#include "non_copyable_movable.h"

namespace taotu {

class Eventer;

/**
 * @brief  // TODO:
 *
 */
class Socketer : NonCopyableMovable {
 public:
  typedef std::function<void()> NormalCallback;
  typedef std::function<void(/*TODO:*/)> ReadCallback;
  typedef std::weak_ptr<Eventer> EventerPtr;

  Socketer(std::shared_ptr<Eventer> eventer, int fd);
  ~Socketer();

  // Handle all events
  void Work(/*TODO:*/);

  void RegisterReadCallBack(ReadCallback cb);
  void RegisterWriteCallback(NormalCallback cb);
  void RegisterCloseCallback(NormalCallback cb);
  void RegisterErrorCallback(NormalCallback cb);

  int Fd() const;
  int Events() const;

  void ReceiveEvents(int in_events);

  bool HasNoEvent() const;
  bool HasReadEvents() const;
  bool HasWriteEvents() const;

  void EnableReadEvents();
  void DisableReadEvents();
  void EnableWriteEvents();
  void DisableWriteEvents();
  void ClearAllEvents();

  std::shared_ptr<Eventer> HostEventer();

  void RemoveMyself();

 private:
  void UpdateEvents();

  enum {
    kNoEvent = 0,
    kReadEvents = 1 | 2,
    kWriteEvents = 4,
  };

  EventerPtr eventer_;

  int fd_;

  // For receiving
  int in_events_;

  // For setting
  int out_events_;

  ReadCallback read_callback_;
  NormalCallback write_callback_;
  NormalCallback close_callback_;
  NormalCallback error_callback_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_SOCKETER_H_
