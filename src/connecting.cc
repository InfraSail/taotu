/**
 * @file connecting.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
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

#include <functional>

#include "event_manager.h"
#include "logger.h"

using namespace taotu;

Connecting::Connecting(EventManager* event_manager, int socket_fd,
                       const NetAddress& local_address,
                       const NetAddress& peer_address)
    : event_manager_(event_manager),
      eventer_(event_manager->GetPoller(), socket_fd),
      socketer_(socket_fd),
      local_address_(local_address),
      peer_address_(peer_address),
      state_(kConnecting) {
  eventer_.RegisterReadCallback(
      std::bind(&Connecting::DoReading, this, std::placeholders::_1));
  eventer_.RegisterWriteCallback(std::bind(&Connecting::DoWriting, this));
  eventer_.RegisterCloseCallback(std::bind(&Connecting::DoClosing, this));
  eventer_.RegisterErrorCallback(std::bind(&Connecting::DoWithError, this));
  // LOG(logger::kDebug, "The TCP connection with fd(" +
  //                         std::to_string(socket_fd) + ") is being created.");
  socketer_.SetKeepAlive(true);
}
Connecting::~Connecting() {}

void Connecting::DoReading(TimePoint receive_time) {
  if (eventer_.HasReadEvents()) {
    int saved_errno = 0;
    ssize_t n = input_buffer_.ReadFromFd(Fd(), &saved_errno);
    if (n > 0) {
      if (OnMessageCallback_) {
        OnMessageCallback_(*this, &input_buffer_, receive_time);
      }
    } else if (0 == n) {
      DoClosing();
    } else {
      errno = saved_errno;
      LOG(logger::kError, "Fd(" + std::to_string(Fd()) + ") reading failed!!!");
      DoWithError();
    }
    if (!(IsConnected())) {
      StopReadingWriting();
    }
  } else {
    DoClosing();
  }
}
void Connecting::DoWriting() {
  if (eventer_.HasWriteEvents()) {
    ssize_t n = output_buffer_.WriteToFd(Fd());
    if (n > 0) {
      if (0 == output_buffer_.GetReadableBytes()) {
        eventer_.DisableWriteEvents();
        if (WriteCompleteCallback_) {
          WriteCompleteCallback_(*this);
        }
        if (kDisconnecting == state_) {
          if (!eventer_.HasWriteEvents()) {
            socketer_.ShutdownWrite();
          }
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
  if (state_ != kDisconnected) {
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
  }
}
void Connecting::DoWithError() {
  int saved_errno = 0, opt_val;
  socklen_t opt_len = static_cast<socklen_t>(sizeof(opt_val));
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
  if (kConnecting == state_) {
    SetState(kConnected);
    StartReading();
    if (OnConnectionCallback_) {
      OnConnectionCallback_(*this);
    }
  }
}
void Connecting::OnDestroying() {
  if (IsConnected()) {
    SetState(kDisconnected);
    StopReadingWriting();
    if (OnConnectionCallback_) {
      OnConnectionCallback_(*this);
    }
  }
}

void Connecting::Send(const void* message, size_t msg_len) {
  if (kDisconnected == state_) {
    LOG(logger::kError,
        "Fd(" + std::to_string(Fd()) +
            ") is disconnected, so give up sending the message!!!");
    return;
  }
  if (kConnected == state_) {
    ssize_t sent_bytes = 0;
    size_t unsent_bytes = msg_len;
    // If there is nothing in "output_buffer_", send the message directly
    bool fault = false;
    if (!eventer_.HasWriteEvents() && output_buffer_.GetReadableBytes() == 0) {
      sent_bytes = ::write(Fd(), message, msg_len);
      if (sent_bytes >= 0) {
        unsent_bytes = msg_len - sent_bytes;
        if (0 == unsent_bytes && WriteCompleteCallback_) {
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
    if (!fault && unsent_bytes > 0) {
      size_t prv_len = output_buffer_.GetReadableBytes();
      if (HighWaterMarkCallback_ &&
          prv_len + unsent_bytes >= high_water_mark_ &&
          prv_len < high_water_mark_) {
        HighWaterMarkCallback_(*this, prv_len + unsent_bytes);
      }
      output_buffer_.Append(
          reinterpret_cast<const void*>(
              reinterpret_cast<char*>(const_cast<void*>(message)) + sent_bytes),
          unsent_bytes);
      StartWriting();
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

void Connecting::ShutDown() {
  if (kConnected == state_) {
    SetState(kDisconnecting);
    if (!(eventer_.HasWriteEvents())) {
      socketer_.ShutdownReadWrite();
    }
  }
}
void Connecting::ShutDownWrite() {
  if (kConnected == state_) {
    SetState(kDisconnecting);
    if (!(eventer_.HasWriteEvents())) {
      socketer_.ShutdownWrite();
    }
  }
}

void Connecting::ForceClose() {
  if (IsConnected() || kDisconnecting == state_) {
    SetState(kDisconnecting);
    DoClosing();
  }
}
void Connecting::ForceCloseAfter(int64_t delay_microseconds) {
  if (IsConnected() || kDisconnecting == state_) {
    event_manager_->RunAfter(delay_microseconds,
                             std::bind(&Connecting::ForceClose, this));
  }
}

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
