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

#include <stddef.h>

#include <functional>
#include <string>
#include <utility>

#include "eventer.h"
#include "io_buffer.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "socketer.h"
#include "time_point.h"
#include "timer.h"

namespace taotu {

class EventManager;

/**
 * @brief  // TODO:
 *
 */
class Connecting : NonCopyableMovable {
 public:
  typedef std::function<void(Connecting&)> NormalCallback;
  typedef std::function<void(Connecting&, IoBuffer*, TimePoint)>
      OnMessageCallback;
  typedef std::function<void(Connecting&, size_t)> HighWaterMarkCallback;

  Connecting(EventManager* event_manager, int socket_fd,
             const NetAddress& local_address, const NetAddress& peer_address);
  ~Connecting();

  void RegisterOnConnectionCallback(NormalCallback cb) {
    OnConnectionCallback_ = std::move(cb);
  }
  void RegisterOnMessageCallback(OnMessageCallback cb) {
    OnMessageCallback_ = std::move(cb);
  }
  void RegisterWriteCallback(NormalCallback cb) {
    WriteCompleteCallback_ = std::move(cb);
  }
  void RegisterHighWaterMarkCallback(HighWaterMarkCallback cb,
                                     size_t high_water_mark) {
    HighWaterMarkCallback_ = std::move(cb);
    high_water_mark_ = high_water_mark;
  }
  void RegisterCloseCallback(NormalCallback cb) {
    CloseCallback_ = std::move(cb);
  }

  void DoReading(TimePoint receive_time);
  void DoWriting();
  void DoClosing();
  void DoWithError();

  void StartReading() {
    if (!eventer_.HasReadEvents()) {
      eventer_.EnableReadEvents();
    }
  }
  void StopReading() {
    if (eventer_.HasReadEvents()) {
      eventer_.DisableReadEvents();
    }
  }
  void StartWriting() {
    if (!eventer_.HasWriteEvents()) {
      eventer_.EnableWriteEvents();
    }
  }
  void StopWriting() {
    if (eventer_.HasWriteEvents()) {
      eventer_.DisableWriteEvents();
    }
  }
  void StopReadingWriting() {
    if (eventer_.HasReadEvents() || eventer_.HasWriteEvents()) {
      eventer_.DisableAllEvents();
    }
  }

  IoBuffer* GetInputBuffer() { return &input_buffer_; }
  IoBuffer* GetOutputBuffer() { return &output_buffer_; }

  // Be called when this connection establishing
  void OnEstablishing();
  // Be called when this connection destroying
  void OnDestroying();

  void Send(const void* message, size_t msg_len);
  void Send(const std::string& message);
  void Send(IoBuffer* io_buffer);

  bool IsConnected() { return kConnected == state_; }
  bool IsDisconnected() { return kDisconnected == state_; }

  void SetTcpNoDelay(bool on) { socketer_.SetTcpNoDelay(on); }

  void ForceClose();
  void ForceCloseAfter(int64_t delay_microseconds);

 private:
  enum ConnectionState {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisconnecting
  };

  void SetState(ConnectionState state) { state_ = state; }

  static std::string GetConnectionStateInfo(ConnectionState state);

  EventManager* event_manager_;

  Eventer eventer_;
  Socketer socketer_;
  const NetAddress local_address_;
  const NetAddress peer_address_;

  NormalCallback OnConnectionCallback_;
  OnMessageCallback OnMessageCallback_;
  NormalCallback WriteCompleteCallback_;
  HighWaterMarkCallback HighWaterMarkCallback_;
  NormalCallback CloseCallback_;

  size_t high_water_mark_;

  IoBuffer input_buffer_;
  IoBuffer output_buffer_;

  ConnectionState state_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTING_H_
