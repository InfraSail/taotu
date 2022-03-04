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
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "logger.h"

using namespace taotu;

Socketer::Socketer(int socket_fd) : socket_fd_(socket_fd) {}
Socketer::~Socketer() {}

void Socketer::BindAddress(const NetAddress& local_address) {
  int ret = ::bind(socket_fd_, local_address.GetNetAddress(),
                   static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                            ") failed to bind an address!!!");
  }
}
void Socketer::Listen() {
  int ret = ::listen(socket_fd_, SOMAXCONN);
  if (ret < 0) {
    LOG(logger::kError,
        "SocketFd(" + std::to_string(socket_fd_) + ") failed to listen!!!");
  }
}
int Socketer::Accept(NetAddress* peer_address) {
  // Ignore whether IP address specification of client-end is IPv4 or IPv6
  struct sockaddr_in6 socket_address6;
  ::memset(&socket_address6, 0, sizeof(socket_address6));
  socklen_t addr_len = static_cast<socklen_t>(sizeof(socket_address6));
  int conn_fd = ::accept4(
      socket_fd_,
      static_cast<struct sockaddr*>(reinterpret_cast<void*>(&socket_address6)),
      &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
        LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                                ") accept: unexpected error!!!");
        break;
      default:
        LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                                ") accept: unknown error!!!");
        break;
    }
  } else {
    peer_address->SetNetAddress6(socket_address6);
  }
  return conn_fd;
}

void Socketer::ShutdownWrite() {
  if (::shutdown(socket_fd_, SHUT_WR) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                            ") failed to shutdown writing end!!!");
  }
}

void Socketer::ShutdownReadWrite() {
  if (::shutdown(socket_fd_, SHUT_RDWR) < 0) {
    LOG(logger::kError,
        "SocketFd(" + std::to_string(socket_fd_) +
            ") failed to shutdown reading end and writing end!!!");
  }
}

void Socketer::Close() {
  // Close the file and give its descriptor back
  LOG(logger::kDebug,
      "SocketFd(" + std::to_string(socket_fd_) + ") is closing.");
  ::close(socket_fd_);
}

void Socketer::SetTcpNoDelay(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                            ") failed to set no delay(TCP) " +
                            (on ? "on" : "off") + " !!!");
  }
}

void Socketer::SetReuseAddress(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                            ") failed to set reuse address " +
                            (on ? "on" : "off") + " !!!");
  }
}

void Socketer::SetReusePort(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEPORT, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                            ") failed to set reuse port " +
                            (on ? "on" : "off") + " !!!");
  }
}

void Socketer::SetKeepAlive(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(socket_fd_, SOL_SOCKET, SO_KEEPALIVE, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd_) +
                            ") failed to set keep alive " +
                            (on ? "on" : "off") + " !!!");
  }
}
