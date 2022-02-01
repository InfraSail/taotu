/**
 * @file connecting.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-27
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_CONNECTING_H_
#define TAOTU_TAOTU_CONNECTING_H_

#include <utility>

#include "eventer.h"
#include "io_buffer.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "socketer.h"
#include "time_point.h"

namespace taotu {

class Poller;

/**
 * @brief  // TODO:
 *
 */
class Connecting : NonCopyableMovable {
 public:
  Connecting(Poller* poller, int socket_fd, const NetAddress& local_address,
             const NetAddress& peer_address);
  ~Connecting();

  void RegisterReadCallback(Eventer::ReadCallback cb) {
    ReadCallback_ = std::move(cb);
  }
  void RegisterWriteCallback(Eventer::NormalCallback cb) {
    WriteCallback_ = std::move(cb);
  }
  void RegisterCloseCallback(Eventer::NormalCallback cb) {
    CloseCallback_ = std::move(cb);
  }
  void RegisterErrorCallback(Eventer::NormalCallback cb) {
    ErrorCallback_ = std::move(cb);
  }

  void DoReading(TimePoint receive_time);
  void DoWriting();
  void DoClosing();
  void DoWithError();

 private:
  Eventer eventer_;
  Socketer socketer_;
  NetAddress local_address_;
  NetAddress peer_address_;

  Eventer::ReadCallback ReadCallback_;
  Eventer::NormalCallback WriteCallback_;
  Eventer::NormalCallback CloseCallback_;
  Eventer::NormalCallback ErrorCallback_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTING_H_
