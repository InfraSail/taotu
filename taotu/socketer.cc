/**
 * @file socketer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  //TODO:
 * @date 2021-12-08
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "socketer.h"

#include <errno.h>
#include <netinet/tcp.h>
#include <strings.h>

#include <string>

#include "logger.h"

using namespace taotu;

Socketer::Socketer(int socket_fd) : fd_(socket_fd) {}
Socketer::~Socketer() {}

int Socketer::Fd() { return fd_; }

void Socketer::BindAddress(const struct sockaddr* local_address) {
  int ret = ::bind(fd_, local_address,
                   static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG(logger::kError,
        "SocketFd(" + std::to_string(fd_) + ") failed to bind an address!!!");
  }
}
void Socketer::Listen() {
  int ret = ::listen(fd_, SOMAXCONN);
  if (ret < 0) {
    LOG(logger::kError,
        "SocketFd(" + std::to_string(fd_) + ") failed to listen!!!");
  }
}
int Socketer::Accept(struct sockaddr_in6* peer_address) {
  struct sockaddr_in6 addr;
  ::memset(&addr, 0, sizeof(addr));
  socklen_t addr_len = static_cast<socklen_t>(sizeof(addr));
  int conn_fd = ::accept4(
      fd_, static_cast<struct sockaddr*>(reinterpret_cast<void*>(&addr)),
      &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (conn_fd < 0) {
    int savedErrno = errno;
    switch (savedErrno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:
      case EPERM:
      case EMFILE:
        errno = savedErrno;
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
        LOG(logger::kError, "SocketFd(" + std::to_string(fd_) +
                                ") accept: unexpected error!!!");
        break;
      default:
        LOG(logger::kError,
            "SocketFd(" + std::to_string(fd_) + ") accept: unknown error!!!");
        break;
    }
  } else {
    *peer_address = addr;
  }
  return conn_fd;
}

void Socketer::ShutdownWrite() {
  if (::shutdown(fd_, SHUT_WR) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(fd_) +
                            ") failed to shutdown writing end!!!");
  }
}

void Socketer::SetTcpNoDelay(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(fd_) +
                            ") failed to set no delay(TCP) " +
                            (on ? "on" : "off") + " !!!");
  }
}

void Socketer::SetReuseAddress(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(fd_) +
                            ") failed to set reuse address " +
                            (on ? "on" : "off") + " !!!");
  }
}

void Socketer::SetReusePort(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(fd_) +
                            ") failed to set reuse port " +
                            (on ? "on" : "off") + " !!!");
  }
}

void Socketer::SetKeepAlive(bool on) {
  int opt = on ? 1 : 0;
  if (::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt,
                   static_cast<socklen_t>(sizeof(opt))) < 0) {
    LOG(logger::kError, "SocketFd(" + std::to_string(fd_) +
                            ") failed to set keep alive " +
                            (on ? "on" : "off") + " !!!");
  }
}
