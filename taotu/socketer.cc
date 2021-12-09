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

#include <netinet/in.h>

#include <string>

#include "logger.h"

using namespace taotu;

Socketer::Socketer(int socket_fd) : fd_(socket_fd) {}
Socketer::~Socketer() {}

int Socketer::Fd() { return fd_; }

void Socketer::BindAddress(const struct sockaddr* addr) {
  int ret =
      ::bind(fd_, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG(logger::kError,
        "Fd(" + std::to_string(fd_) + ") failed to bind an address!!!");
  }
}
