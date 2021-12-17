/**
 * @file reactor.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "reactor.h"

#include <stdlib.h>

#include "logger.h"

using namespace taotu;

Reactor::Reactor(SocketAddress& listen_address)
    : acceptor_(listen_address, true) {
  if (acceptor_.Fd() >= 0 && !acceptor_.IsListening()) {
    acceptor_.Listen();
  } else {
    LOG(logger::kError, "Fail to init the  acceptor!!!");
    ::exit(-1);
  }
}
Reactor::~Reactor() {}
