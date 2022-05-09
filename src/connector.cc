/**
 * @file connector.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Connector" which is the connector of new TCP
 * connections requests the server and create new connections.
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */
#include "connector.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <string>

#include "connecting.h"
#include "logger.h"
#include "time_point.h"

using namespace taotu;
enum {
  kMaxRetryDelayMicroseconds = 30 * 1000 * 1000,
  kInitRetryDelayMicroseconds = 500 * 1000,
};
static struct sockaddr_in6 GetLocalSocketAddress6(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  socklen_t addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getsockname(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG(logger::kError, "Fail to get local network info when accepting!!!");
  }
  return local_addr;
}
static struct sockaddr_in6 GetPeerSocketAddress6(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  socklen_t addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getpeername(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG(logger::kError, "Fail to get local network info when accepting!!!");
  }
  return local_addr;
}
static int GetSocketError(int socket_fd) {
  int socket_option;
  socklen_t socket_option_length =
      static_cast<socklen_t>(sizeof(socket_option));
  if (::getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_option,
                   &socket_option_length) < 0) {
    return errno;
  } else {
    return socket_option;
  }
}
Connector::Connector(EventManager* event_manager,
                     const NetAddress& server_address)
    : event_manager_(event_manager),
      server_address_(server_address),
      state_(kDisconnected),
      can_connect_(false),
      retry_dalay_microseconds_(static_cast<int>(kInitRetryDelayMicroseconds)) {
}

void Connector::Start() {
  can_connect_ = true;
  Connect();
}
void Connector::Restart() {
  SetState(kDisconnected);

  retry_dalay_microseconds_ = static_cast<int>(kInitRetryDelayMicroseconds);
  Start();
}
void Connector::Stop() {
  can_connect_ = false;
  if (kConnecting == state_) {
    SetState(kDisconnected);
    DoRetrying(RemoveAndReset());
  }
}
void Connector::Connect() {
  int sock_fd =
      ::socket(server_address_.GetFamily(),
               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
#ifndef __linux__
  int flags = ::fcntl(sock_fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  ::fcntl(sock_fd, F_SETFL, flags);
  flags = ::fcntl(sock_fd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ::fcntl(sock_fd, F_SETFD, flags);
#endif
  if (sock_fd < 0) {
    LOG(logger::kError, "Fail to initialize for connector!!!");
  }
  int status = ::connect(sock_fd, server_address_.GetNetAddress(),
                         static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  int saved_errno = (0 == status) ? 0 : errno;
  switch (saved_errno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      DoConnecting(sock_fd);
      break;
    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      DoRetrying(sock_fd);
      break;
    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG(logger::kError, "Connector fd(%d) is closing because of an error!!!",
          sock_fd);
      ::close(sock_fd);
      break;
    default:
      LOG(logger::kError,
          "Connector fd(%d) is closing because of an unkown error(%d)!!!",
          sock_fd, saved_errno);
      ::close(sock_fd);
      break;
  }
}
void Connector::DoConnecting(int conn_fd) {
  SetState(kConnecting);
  eventer_ = std::make_unique<Eventer>(event_manager_->GetPoller(), conn_fd);
  eventer_->RegisterWriteCallback([this]() { this->DoWriting(); });
  eventer_->RegisterErrorCallback([this]() { this->DoWithError(); });
  eventer_->EnableWriteEvents();
}
void Connector::DoRetrying(int conn_fd) {
  LOG(logger::kWarn, "Connector fd(%d) is closing for retrying!", conn_fd);
  ::close(conn_fd);
  SetState(kDisconnected);
  if (can_connect_) {
    // LOG(logger::kDebug, "Connector fd(%d) is retrying to connect.",
    // conn_fd);
    event_manager_->RunAfter(retry_dalay_microseconds_,
                             [this]() { this->Start(); });
    retry_dalay_microseconds_ =
        std::min(retry_dalay_microseconds_ * 2,
                 static_cast<int>(kMaxRetryDelayMicroseconds));
  } else {
    // LOG(logger::kDebug, "Connector fd(%d) is not retrying to connect.",
    //     conn_fd);
  }
}
void Connector::DoWriting() {
  LOG(logger::kDebug, "Connector fd(%d) is writing.", eventer_->Fd());
  if (kConnecting == state_) {
    int conn_fd = RemoveAndReset();
    int error = GetSocketError(conn_fd);
    if (error) {
      char errno_info[512];
      LOG(logger::kWarn, "Connector fd(%d) has the error(%s)!", conn_fd,
          ::strerror_r(error, errno_info, sizeof(errno_info)));
      DoRetrying(conn_fd);
    } else if ([](int conn_fd) -> bool {
                 struct sockaddr_in6 local_address =
                     GetLocalSocketAddress6(conn_fd);
                 struct sockaddr_in6 peer_address =
                     GetPeerSocketAddress6(conn_fd);
                 if (local_address.sin6_family == AF_INET) {
                   const struct sockaddr_in* laddr4 =
                       reinterpret_cast<struct sockaddr_in*>(&local_address);
                   const struct sockaddr_in* raddr4 =
                       reinterpret_cast<struct sockaddr_in*>(&peer_address);
                   return laddr4->sin_port == raddr4->sin_port &&
                          laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
                 } else if (local_address.sin6_family == AF_INET6) {
                   return local_address.sin6_port == peer_address.sin6_port &&
                          ::memcmp(&local_address.sin6_addr,
                                   &peer_address.sin6_addr,
                                   sizeof local_address.sin6_addr) == 0;
                 } else {
                   return false;
                 }
               }(conn_fd)) {  // Check whether it is self-connected
      // LOG(logger::kDebug, "Connector fd(%d) is self-connected.", conn_fd);
      DoRetrying(conn_fd);
    } else {
      SetState(kConnected);
      if (can_connect_ && NewConnectionCallback_) {
        NewConnectionCallback_(conn_fd);
      } else {
        ::close(conn_fd);
      }
    }
  }
}
void Connector::DoWithError() {
  LOG(logger::kError, "Connector fd(%d) has the error with the state(%d).",
      eventer_->Fd(), state_);
  if (kConnecting == state_) {
    int conn_fd = RemoveAndReset();
    int error = GetSocketError(conn_fd);
    char errno_info[512];
    LOG(logger::kWarn, "Connector fd(%d) has the error(%s)!", conn_fd,
        ::strerror_r(error, errno_info, sizeof(errno_info)));
    DoRetrying(conn_fd);
  }
}
int Connector::RemoveAndReset() {
  eventer_->DisableAllEvents();
  int conn_fd = eventer_->Fd();
  eventer_->GetReadyDestroy();  // Set Eventer::is_handling_ flag off
  eventer_.reset();
  return conn_fd;
}
