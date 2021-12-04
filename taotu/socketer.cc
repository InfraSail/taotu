/**
 * @file socketer.cc
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief  // TODO:
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "socketer.h"

#include "logger.h"

using namespace taotu;

Socketer::Socketer(std::shared_ptr<Eventer> eventer, int fd) {}

Socketer::~Socketer() {}

int Socketer::Fd() const { return fd_; }

int Socketer::Events() const { return out_events_; }
