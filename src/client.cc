/**
 * @file client.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Client" which is the encapsulation of the
 * client.
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "client.h"

using namespace taotu;

Client::Client(EventManager* event_manager, const NetAddress& server_address,
               bool should_retry_)
    : reactor_manager_(std::make_unique<ClientReactorManager>(event_manager,
                                                              server_address)) {
  reactor_manager_->SetRetryOn(should_retry_);
}
Client::~Client() {}

void Client::SetConnectionCallback(const std::function<void(Connecting&)>& cb) {
  reactor_manager_->SetConnectionCallback(std::move(cb));
}
void Client::SetMessageCallback(
    const std::function<void(Connecting&, IoBuffer*, TimePoint)>& cb) {
  reactor_manager_->SetMessageCallback(std::move(cb));
}
void Client::SetWriteCompleteCallback(
    const std::function<void(Connecting&)>& cb) {
  reactor_manager_->SetWriteCompleteCallback(std::move(cb));
}

void Client::Connect() { reactor_manager_->Connect(); }
void Client::Disconnect() { reactor_manager_->Disconnect(); }
void Client::Stop() { reactor_manager_->Stop(); }
