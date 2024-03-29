/**
 * @file socketer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Socketer" which is the encapsulation of
 * socket file descriptor.
 * @date 2021-12-08
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "socketer.h"

#include <errno.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <string>

#include "logger.h"

using namespace taotu;

Socketer::Socketer(int socket_fd) : socket_fd_(socket_fd) {}
Socketer::~Socketer() { Close(); }

void Socketer::BindAddress(const NetAddress& local_address) const {
  int ret = ::bind(socket_fd_, local_address.GetNetAddress(),
                   static_cast<socklen_t>(local_address.GetSize()));
  if (ret < 0) {
    LOG_ERROR("SocketFd(%d) failed to bind an address!!!", socket_fd_);
  }
}
void Socketer::Listen() const {
  int ret = ::listen(socket_fd_, SOMAXCONN);
  if (ret < 0) {
    LOG_ERROR("SocketFd(%d) failed to listen!!!", socket_fd_);
  }
}
int Socketer::Accept(NetAddress* peer_address) const {
#ifndef __linux__
  struct sockaddr_in6 socket_address6 {};
  struct sockaddr_in socket_address {};
  auto addr_len = static_cast<socklen_t>(peer_address->GetSize());
  void* sockaddr_ptr = nullptr;
  if (peer_address->GetFamily() == AF_INET6) {
    sockaddr_ptr = reinterpret_cast<void*>(&socket_address6);
  } else {
    sockaddr_ptr = reinterpret_cast<void*>(&socket_address);
  }
  int conn_fd = ::accept(
      socket_fd_, static_cast<struct sockaddr*>(sockaddr_ptr), &addr_len);
  int flags = ::fcntl(conn_fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  ::fcntl(conn_fd, F_SETFL, flags);
  flags = ::fcntl(conn_fd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ::fcntl(conn_fd, F_SETFD, flags);
#else
  // Ignore whether IP address specification of client-end is IPv4 or IPv6
  struct sockaddr_in6 socket_address6 {};
  ::memset(&socket_address6, 0, sizeof(socket_address6));
  auto addr_len = static_cast<socklen_t>(sizeof(socket_address6));
  int conn_fd = ::accept4(
      socket_fd_,
      static_cast<struct sockaddr*>(reinterpret_cast<void*>(&socket_address6)),
      &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  if (conn_fd < 0) {  // Error occurs
    int saved_errno = errno;
    switch (saved_errno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:
      case EPERM:
      case EMFILE:
        errno = saved_errno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        LOG_ERROR("SocketFd(%d) accept: unexpected error!!!", socket_fd_);
        break;
      default:
        LOG_ERROR("SocketFd(%d) accept: unknown error!!!", socket_fd_);
        break;
    }
  } else {
#ifndef __linux__
    if (peer_address->GetFamily() == AF_INET6) {
      peer_address->SetNetAddress6(socket_address6);
    } else {
      peer_address->SetNetAddress(socket_address);
    }
#else
    peer_address->SetNetAddress6(socket_address6);
#endif
  }
  return conn_fd;
}

void Socketer::ShutdownWrite() const {
  if (::shutdown(socket_fd_, SHUT_WR) < 0) {
    LOG_ERROR("SocketFd(%d) failed to shutdown writing end!!!", socket_fd_);
  }
}

void Socketer::SetTcpNoDelay(bool on) const {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG_ERROR("SocketFd(%d) failed to set no delay(TCP) %s!!!", socket_fd_,
              (on ? "on" : "off"));
  }
}

void Socketer::SetReuseAddress(bool on) const {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG_ERROR("SocketFd(%d) failed to set reuse address %s!!!", socket_fd_,
              (on ? "on" : "off"));
  }
}

void Socketer::SetReusePort(bool on) const {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEPORT, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG_ERROR("SocketFd(%d) failed to set reuse port %s!!!", socket_fd_,
              (on ? "on" : "off"));
  }
}

void Socketer::SetKeepAlive(bool on) const {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, SOL_SOCKET, SO_KEEPALIVE, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG_ERROR("SocketFd(%d) failed to set keep alive %s!!!", socket_fd_,
              (on ? "on" : "off"));
  }
}

void Socketer::Close() const {
  LOG_DEBUG("SocketFd(%d) is closing.", socket_fd_);

  if (socket_fd_ != -1) {
    // Close the file and give its descriptor back
    ::close(socket_fd_);
    socket_fd_ = -1;
  }
}
