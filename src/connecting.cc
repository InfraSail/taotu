/**
 * @file connecting.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Connecting" which is a TCP connection
 * (status).
 * @date 2021-12-27
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "connecting.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "event_manager.h"
#include "logger.h"

using namespace taotu;

Connecting::Connecting(EventManager* event_manager, int socket_fd,
                       const NetAddress& local_address,
                       const NetAddress& peer_address)
    : event_manager_(event_manager),
      socketer_(socket_fd),
      eventer_(event_manager->GetPoller(), socket_fd),
      local_address_(local_address),
      peer_address_(peer_address),
      state_(kConnecting) {
  socketer_.SetKeepAlive(true);
  eventer_.RegisterReadCallback(
      [this](TimePoint receive_time) { this->DoReading(receive_time); });
  eventer_.RegisterWriteCallback([this] { this->DoWriting(); });
  eventer_.RegisterCloseCallback([this] { this->DoClosing(); });
  eventer_.RegisterErrorCallback([this] { this->DoWithError(); });
  // LOG(logger::kDebug, "The TCP connection with fd(" +
  //                         std::to_string(socket_fd) + ") is being created.");
}
Connecting::~Connecting() {
  // LOG(logger::kDebug,
  //     "The TCP connection with fd(" + std::to_string(Fd()) + ") is
  //     closing.");
}

void Connecting::DoReading(TimePoint receive_time) {
  int saved_errno = 0;
  ssize_t n =
      input_buffer_.ReadFromFd(Fd(), &saved_errno);  // Receive data truly
  if (n > 0) {  // If some data received, try to call the corresponding callback
                // function
    if (OnMessageCallback_) {
      OnMessageCallback_(*this, &input_buffer_, receive_time);
    }
  } else if (0 == n) {  // Close when no data received
    DoClosing();
  } else {  // Handle the error
    errno = saved_errno;
    LOG(logger::kError, "Fd(" + std::to_string(Fd()) + ") reading failed!!!");
    DoWithError();
  }
  if (!IsConnected()) {  // If not connected, stop reading and writing
    StopReadingWriting();
  }
}
void Connecting::DoWriting() {
  if (eventer_.HasWriteEvents()) {
    ssize_t n = output_buffer_.WriteToFd(Fd());  // Send data truly
    if (n > 0) {
      if (0 == output_buffer_.GetReadableBytes()) {
        eventer_.DisableWriteEvents();
        if (WriteCompleteCallback_) {
          WriteCompleteCallback_(*this);
        }
        if (kDisconnecting == state_.load() &&
            !eventer_
                 .HasWriteEvents()) {  // If the state of this TCP connection is
                                       // disconnecting and the writing event of
                                       // this TCP connection is not paid
                                       // attention to, shut down the writing
                                       // end (this end)
          socketer_.ShutdownWrite();
        }
      }
    } else {
      LOG(logger::kError, "Fd(" + std::to_string(Fd()) + ") writing failed!!!");
    }
  } else {
    LOG(logger::kWarn,
        "Fd(" + std::to_string(Fd()) +
            ") should not be retried anymore because it is down!");
  }
}
void Connecting::DoClosing() {
  if (state_.load() != kDisconnected) {
    // LOG(logger::kDebug, "Fd(" + std::to_string(Fd()) + ") with state(\"" +
    //                         GetConnectionStateInfo(state_) + "\") is
    //                         closing.");
    SetState(kDisconnected);
    StopReadingWriting();
    if (OnConnectionCallback_) {
      OnConnectionCallback_(*this);
    }
    if (CloseCallback_) {
      CloseCallback_(*this);
    }
    event_manager_->DeleteConnection(
        Fd());  // Postpone the real destroying to the end of this loop
  }
}
void Connecting::DoWithError() const {
  int opt_val;
  auto opt_len = static_cast<socklen_t>(sizeof(opt_val));
  int saved_errno = 0;
  if (::getsockopt(Fd(), SOL_SOCKET, SO_ERROR,
                   reinterpret_cast<void*>(&opt_val), &opt_len) < 0) {
    saved_errno = errno;
  } else {
    saved_errno = opt_val;
  }
  char errno_info[512];
  auto tmp_ptr = ::strerror_r(saved_errno, errno_info, sizeof(errno_info));
  LOG(logger::kError, "Fd(" + std::to_string(Fd()) + ") gets an error -- " +
                          std::string{tmp_ptr} + "!!!");
}

void Connecting::OnEstablishing() {
  if (kConnecting ==
      state_.load()) {  // This TCP connection can only be create once
    SetState(kConnected);
    StartReading();
  }
}

void Connecting::Send(const void* message, size_t msg_len) {
  if (kDisconnected == state_.load()) {
    LOG(logger::kError,
        "Fd(" + std::to_string(Fd()) +
            ") is disconnected, so give up sending the message!!!");
    return;
  }
  if (kConnected == state_.load()) {
    ssize_t sent_bytes = 0;
    size_t unsent_bytes = msg_len;
    bool fault = false;  // 'false' represents that there is no error
    if (!eventer_.HasWriteEvents() &&
        output_buffer_.GetReadableBytes() ==
            0) {  // If there is nothing in the buffer for output, send the
                  // message directly
      sent_bytes = ::send(Fd(), message, msg_len, MSG_NOSIGNAL);
      if (sent_bytes >= 0) {  // If several bytes were sent successfully
        unsent_bytes = msg_len - sent_bytes;
        if (0 == unsent_bytes &&
            WriteCompleteCallback_) {  // If all bytes were sent successfully
          WriteCompleteCallback_(*this);
        }
      } else {
        sent_bytes = 0;
        if (EWOULDBLOCK != errno) {
          LOG(logger::kWarn, "Cannot send the message to fd(" +
                                 std::to_string(Fd()) + ") directly now!");
          if (EPIPE == errno || ECONNRESET == errno) {
            fault = true;
          }
        }
      }
    }
    if (!fault &&
        unsent_bytes > 0) {  // If several bytes were not sent successfully
      size_t prv_len = output_buffer_.GetReadableBytes();
      if (HighWaterMarkCallback_ &&
          prv_len + unsent_bytes >= high_water_mark_ &&
          prv_len <
              high_water_mark_) {  // If the high mark threshold was reached up
                                   // to, call the callback function
        HighWaterMarkCallback_(*this, prv_len + unsent_bytes);
      }
      output_buffer_.Append(
          reinterpret_cast<const void*>(
              reinterpret_cast<char*>(const_cast<void*>(message)) + sent_bytes),
          unsent_bytes);  // Append these bytes to the buffer for output
      StartWriting();  // Normally the writing event is not paid attention to,
                       // unless there are data in the buffer for output waiting
                       // for being sent
    }
  }
}
void Connecting::Send(const std::string& message) {
  Send(static_cast<const void*>(message.c_str()), message.size());
}
void Connecting::Send(IoBuffer* io_buffer) {
  Send(io_buffer->GetReadablePosition(), io_buffer->GetReadableBytes());
  io_buffer->RefreshRW();
}

void Connecting::ShutDownWrite() {
  if (kConnected == state_.load()) {
    SetState(kDisconnecting);
    if (!eventer_
             .HasWriteEvents()) {  // If the writing event is not paid attention
                                   // to, shut down the writing end (this end)
      socketer_.ShutdownWrite();
    }
  }
}

void Connecting::ForceClose() {
  if (kDisconnected != state_.load()) {
    SetState(kDisconnecting);
    DoClosing();
  }
}
// FIXME: Make it be effective in the condition that the connection has been
// destroyed.
// void Connecting::ForceCloseAfter(int64_t delay_microseconds) {
//   if (kDisconnected != state_) {
//     event_manager_->RunAfter(delay_microseconds,
//                              std::bind(&Connecting::ForceClose, this));
//   }
// }

std::string Connecting::GetConnectionStateInfo(ConnectionState state) {
  switch (state) {
    case kDisconnected:
      return "Disconnected";
    case kConnecting:
      return "Connecting";
    case kConnected:
      return "Connected";
    case kDisconnecting:
      return "Disconnecting";
  }
  return std::string{};
}
