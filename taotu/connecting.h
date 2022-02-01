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

#include <cstddef>
#include <functional>
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
  typedef std::function<void(const Connecting&)> NormalCallback;
  typedef std::function<void(const Connecting&, IoBuffer*, TimePoint)>
      OnMessageCallback;
  typedef std::function<void(const Connecting&, size_t)> HighWaterMarkCallback;

  Connecting(Poller* poller, int socket_fd, const NetAddress& local_address,
             const NetAddress& peer_address);
  ~Connecting();

  void RegisterOnConnectionCallback(NormalCallback cb) {
    OnConnectionCallback_ = std::move(cb);
  }
  void RegisterOnMessageCallback(OnMessageCallback cb) {
    OnMessageCallback_ = std::move(cb);
  }
  void RegisterWriteCallback(NormalCallback cb) {
    WriteCallback_ = std::move(cb);
  }
  void RegisterHighWaterMarkCallback(HighWaterMarkCallback cb) {
    HighWaterMarkCallback_ = std::move(cb);
  }
  void RegisterCloseCallback(NormalCallback cb) {
    CloseCallback_ = std::move(cb);
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

  NormalCallback OnConnectionCallback_;
  OnMessageCallback OnMessageCallback_;
  NormalCallback WriteCallback_;
  HighWaterMarkCallback HighWaterMarkCallback_;
  NormalCallback CloseCallback_;

  IoBuffer InputBuffer;
  IoBuffer OutputBuffer;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTING_H_
