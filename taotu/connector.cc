/**
 * @file connector.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "connector.h"

#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

#include "eventer.h"
#include "logger.h"

using namespace taotu;

Connector::Connector(Poller* poller, const NetAddress& server_address)
    : server_address_(server_address), state_(kDisconnected) {
  int socket_fd =
      ::socket(server_address_.GetFamily(),
               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (socket_fd < 0) {
    LOG(logger::kError, "Fail to initialize for connector!!!");
    ::exit(-1);
  }
  eventer_ = std::make_unique<Eventer>(poller, socket_fd);
}

void Connector::Connect() {
  int socket_fd = eventer_->Fd();
  int status = ::connect(socket_fd, server_address_.GetNetAddress(),
                         static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  int saved_errno = (0 == status) ? 0 : errno;
  switch (saved_errno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      DoConnecting(socket_fd);
      break;
    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      DoRetrying(socket_fd);
      break;
    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd) +
                              ") is closing because of an error!!!");
      ::close(socket_fd);
      break;
    default:
      LOG(logger::kError, "SocketFd(" + std::to_string(socket_fd) +
                              ") is closing because of an unkown error(" +
                              std::to_string(saved_errno) + ")!!!");
      ::close(socket_fd);
      break;
  }
}
void Connector::DoConnecting(int socket_fd) {
  SetState(kConnecting);
  eventer_ = std::make_unique<Eventer>(poller_, socket_fd);
  eventer_->RegisterWriteCallback(std::bind(&Connector::DoWriting, this));
  eventer_->RegisterErrorCallback(std::bind(&Connector::DoWithError, this));
  eventer_->EnableWriteEvents();
}
void Connector::DoRetrying(int socket_fd) {
  LOG(logger::kWarn,
      "SocketFd(" + std::to_string(socket_fd) + ") is closing for retrying!");
  ::close(socket_fd);
  SetState(kDisconnected);
  Connect();
}

void Connector::DoWriting() {}
void Connector::DoWithError() {}
