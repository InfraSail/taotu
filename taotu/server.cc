/**
 * @file server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "server.h"

#include "reactor_manager.h"

using namespace taotu;

Server::Server(const NetAddress& listen_address, int io_thread_amount,
               bool should_reuse_port)
    : reactor_(std::make_unique<ReactorManager>(
          listen_address, io_thread_amount, should_reuse_port)) {}
Server::~Server() {}
