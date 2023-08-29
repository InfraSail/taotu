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

void RpcServer::RegisterService(::google::protobuf::Service* service) {
  const auto service_descriptor = service->GetDescriptor();
  services_[service_descriptor->name()] = service;
}
void RpcServer::Start() { server_.Start(); }

void RpcServer::OnConnectionCallback(Connecting& connection) {
  LOG_NOTICE("RpcServer - [ IP(%s), Port(%s) ] -> [ IP(%s), Port(%s) ] %s",
             connection.GetLocalNetAddress().GetIp().c_str(),
             std::to_string(connection.GetLocalNetAddress().GetPort()).c_str(),
             connection.GetPeerNetAddress().GetIp().c_str(),
             std::to_string(connection.GetPeerNetAddress().GetPort()).c_str(),
             (connection.IsConnected() ? "UP" : "DOWN"));
  if (connection.IsConnected()) {
    std::shared_ptr<RpcAsyncChannel> rpc_channel =
        std::make_shared<RpcAsyncChannel>(connection);
    rpc_channel->SetServices(&services_);
    connection.RegisterOnMessageCallback([rpc_channel](Connecting& connection,
                                                       IoBuffer* io_buffer,
                                                       TimePoint receive_time) {
      rpc_channel->OnMessage(connection, io_buffer, receive_time);
    });
    connection.SetContext<std::shared_ptr<RpcAsyncChannel>>(rpc_channel);
  } else {
    connection.SetContext<std::shared_ptr<RpcAsyncChannel>>(
        std::shared_ptr<RpcAsyncChannel>());
  }
}
