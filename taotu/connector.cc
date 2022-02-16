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

#include "logger.h"

using namespace taotu;

Connector::Connector(Poller* poller, const NetAddress& server_address)
    : server_address_(server_address), state_(kConnecting) {
  int socket_fd =
      ::socket(server_address_.GetFamily(),
               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (socket_fd < 0) {
    LOG(logger::kError, "Fail to initialize for connector!!!");
    ::exit(-1);
  }
  eventer_ = std::make_unique<Eventer>(poller, socket_fd);
}
Connector::~Connector() {}

void Connector::Connect() {
  // TODO:
}
