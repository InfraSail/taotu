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

#include <functional>
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
