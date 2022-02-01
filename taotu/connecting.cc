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
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iterator>
#include <string>

#include "logger.h"

using namespace taotu;

Connecting::Connecting(Poller* poller, int socket_fd,
                       const NetAddress& local_address,
                       const NetAddress& peer_address)
    : eventer_(poller, socket_fd),
      socketer_(socket_fd),
      local_address_(local_address),
      peer_address_(peer_address) {
  eventer_.RegisterReadCallback(
      std::bind(&Connecting::DoReading, this, std::placeholders::_1));
  eventer_.RegisterWriteCallback(std::bind(&Connecting::DoWriting, this));
  eventer_.RegisterCloseCallback(std::bind(&Connecting::DoClosing, this));
  eventer_.RegisterErrorCallback(std::bind(&Connecting::DoWithError, this));
  LOG(logger::kDebug, "The TCP connection to fd(" + std::to_string(socket_fd) +
                          ") is being created.");
  socketer_.SetKeepAlive(true);
}
Connecting::~Connecting() {
  LOG(logger::kDebug, "The TCP connection to fd(" +
                          std::to_string(socketer_.Fd()) +
                          ") is being closed.");
}

void Connecting::DoReading(TimePoint receive_time) {
  int saved_errno = 0;
  ssize_t n = input_buffer_.ReadFromFd(socketer_.Fd(), &saved_errno);
  if (n > 0) {
    OnMessageCallback_(*this, &input_buffer_, receive_time);
  } else if (0 == n) {
    DoClosing();
  } else {
    errno = saved_errno;
    LOG(logger::kError,
        "Fd(" + std::to_string(socketer_.Fd()) + ") reading failed!!!");
    DoWithError();
  }
}
void Connecting::DoWriting() {
  if (eventer_.HasWriteEvents()) {
    ssize_t n = output_buffer_.WriteToFd(eventer_.Fd());
    if (n > 0) {
      output_buffer_.Refresh(n);
      if (0 == output_buffer_.GetReadableBytes()) {
        eventer_.DisableWriteEvents();
        if (WriteCallback_) {
          WriteCallback_(*this);
        }
        if (kDisconnecting == state_) {
          if (!eventer_.HasWriteEvents()) {
            socketer_.ShutdownWrite();
          }
        }
      }
    } else {
      LOG(logger::kError,
          "Fd(" + std::to_string(socketer_.Fd()) + ") writing failed!!!");
    }
  } else {
    LOG(logger::kWarn,
        "Fd(" + std::to_string(socketer_.Fd()) +
            ") should not be retried anymore because it is down!");
  }
}
void Connecting::DoClosing() {
  LOG(logger::kDebug, "Fd(" + std::to_string(socketer_.Fd()) +
                          ") with state(\"" + ([this]() -> std::string {
                            switch (this->state_) {
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
                          })() +
                          "\") is closiong.");
  SetState(kDisconnected);
  eventer_.DisableAllEvents();
  OnConnectionCallback_(*this);
  CloseCallback_(*this);
}
void Connecting::DoWithError() {
  int saved_errno = 0, opt_val;
  socklen_t opt_len = static_cast<socklen_t>(sizeof(opt_val));
  if (::getsockopt(socketer_.Fd(), SOL_SOCKET, SO_ERROR,
                   reinterpret_cast<void*>(&opt_val), &opt_len) < 0) {
    saved_errno = errno;
  } else {
    saved_errno = opt_val;
  }
  char errno_info[512];
  ::strerror_r(saved_errno, errno_info, sizeof(errno_info));
  LOG(logger::kError,
      "Fd(" + std::to_string(socketer_.Fd()) + ") gets an error -- " + '.');
}