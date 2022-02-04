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

#include "balancer.h"
#include "logger.h"

using namespace taotu;

Reactor::Reactor(NetAddress& listen_address, int thread_amout)
    : acceptor_(listen_address, true) {
  if (acceptor_.Fd() >= 0 && !acceptor_.IsListening()) {
    acceptor_.Listen();
  } else {
    LOG(logger::kError, "Fail to init the acceptor!!!");
    ::exit(-1);
  }
  for (int i = 0; i < thread_amout; ++i) {
    event_managers_.emplace_back(new EventManager);
  }
  balancer_ = std::make_unique<Balancer>(&event_managers_);
}
Reactor::~Reactor() {
  int thread_amout = event_managers_.size();
  for (int i = 0; i < thread_amout; ++i) {
    delete event_managers_[i];
    event_managers_[i] = nullptr;
  }
  event_managers_.clear();
}

void Reactor::Loop() {
  // TODO:
}
