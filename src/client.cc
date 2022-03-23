/**
 * @file client.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
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

void Client::Connect() { reactor_manager_->Connect(); }
void Client::Disconnect() { reactor_manager_->Disconnect(); }
void Client::Stop() { reactor_manager_->Stop(); }
