/**
 * @file rpc_server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "RpcServer" which the encapsulation of the RPC
 * server.
 * @date 2023-08-13
 *
 * @copyright Copyright (c) 2023 Sigma711
 *
 */

#include "rpc_server.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "logger.h"
#include "rpc_channel.h"
#include "server.h"

using namespace taotu;

RpcServer::RpcServer(EventManagers* event_managers,
                     const NetAddress& listen_address)
    : server_(event_managers, listen_address) {
  server_.SetConnectionCallback([this](Connecting& connection) {
    this->OnConnectionCallback(connection);
  });
}

void RpcServer::RegisterService(::google::protobuf::Service*) {}
void RpcServer::Start() {}

void RpcServer::OnConnectionCallback(Connecting& connection) {}
