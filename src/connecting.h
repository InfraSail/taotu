/**
 * @file connecting.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Connecting" which is a TCP connection (status).
 * @date 2021-12-27
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_CONNECTING_H_
#define TAOTU_SRC_CONNECTING_H_

#include <stddef.h>

#include <any>
#include <atomic>
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
 * @brief "Connecting" offers a series of APIs for users. Whether writing the
 * server or client, it will always be a significant utility part for custom
 * logic code of coders. It gives the functions of customizing the way to handle
 * reading, writing and closing, deciding the events which should be paid
 * attention to, forcibly closing, message sending (asynchronously at most of
 * time), shut down the writing end (this end) and so on. Moreover, it gives you
 * rights to operate the buffers for input and output.
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

  int Fd() const { return eventer_.Fd(); }

  const NetAddress& GetLocalNetAddress() const { return local_address_; }
  const NetAddress& GetPeerNetAddress() const { return peer_address_; }

  EventManager& GetEventManager() { return *event_manager_; }

  void SetContext(const std::any& context) { context_ = context; }
  const std::any& GetConstContext() const { return context_; }
  std::any& GetMutableContext() { return context_; }

  void RegisterOnConnectionCallback(const NormalCallback& cb) {
    OnConnectionCallback_ = cb;
  }
  void RegisterOnMessageCallback(const OnMessageCallback& cb) {
    OnMessageCallback_ = cb;
  }
  void RegisterWriteCallback(const NormalCallback& cb) {
    WriteCompleteCallback_ = cb;
  }
  void RegisterHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                     size_t high_water_mark) {
    HighWaterMarkCallback_ = cb;
    high_water_mark_ = high_water_mark;
  }
  void RegisterCloseCallback(const NormalCallback& cb) { CloseCallback_ = cb; }

  // Execute when the reading event happens
  void DoReading(TimePoint receive_time);

  // Execute when the writing event happens
  void DoWriting();

  // Execute when this TCP connection ought to be closed
  void DoClosing();

  // Execute when error happens
  void DoWithError() const;

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

  // Send the message (asynchronously at most of time)
  void Send(const void* message, size_t msg_len);

  // Send the message (asynchronously at most of time)
  void Send(const std::string& message);

  // Send the message (asynchronously at most of time)
  void Send(IoBuffer* io_buffer);

  // Shut down the writing end (close half == stop writing indeed)
  void ShutDownWrite();

  bool IsConnected() const { return kConnected == state_.load(); }
  bool IsDisconnected() const { return kDisconnected == state_.load(); }

  void SetTcpNoDelay(bool on) { socketer_.SetTcpNoDelay(on); }

  // Close this TCP connection directly (at the end of this loop)
  void ForceClose();
  // FIXME: void ForceCloseAfter(int64_t delay_microseconds);

 private:
  enum ConnectionState {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisconnecting,
  };

  // Set the state of this TCP connection
  void SetState(ConnectionState state) { state_.store(state); }

  // For logging (display the connection state)
  static std::string GetConnectionStateInfo(ConnectionState state);

  // Reference to its master event manager in its thread
  EventManager* event_manager_;

  // To manage the socket of its own
  Socketer socketer_;

  // To manage the event of its own
  Eventer eventer_;

  // Record of the local net address info
  const NetAddress local_address_;

  // Record of the peer net address info
  const NetAddress peer_address_;

  // Callback function which will be called after this TCP connection creating
  // and before this TCP connection destroying
  NormalCallback OnConnectionCallback_;

  // Callback function which will be called after each reading
  OnMessageCallback OnMessageCallback_;

  // Callback function which will be called after each real writing
  NormalCallback WriteCompleteCallback_;

  // Callback function which will be called when the I/O buffer for output
  // reaches up to a threshold
  HighWaterMarkCallback HighWaterMarkCallback_;

  // Callback function which will be called when this TCP connection should be
  // closed
  NormalCallback CloseCallback_;

  // Threshold that the high mark callback function will be called when the I/O
  // buffer for output reaches up to
  size_t high_water_mark_;

  // I/O buffer for input
  IoBuffer input_buffer_;

  // I/O buffer for output
  IoBuffer output_buffer_;

  // Connection state (atomic)
  std::atomic<ConnectionState> state_;

  // Context for any object bound
  std::any context_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_CONNECTING_H_
