/**
 * @file rpc_channel.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "RpcChannel" which represents the single RPC
 * message communication pipe.
 * @date 2023-07-21
 *
 * @copyright Copyright (c) 2023 Sigma711
 *
 */

#include "rpc_channel.h"

#include <google/protobuf/descriptor.h>

#include "logger.h"
#include "rpc.pb.h"
#include "spin_lock.h"

using namespace taotu;

namespace taotu {

const char kRpcTag[] = "RPC0";

}  // namespace taotu

RpcChannel::RpcChannel()
    : codec_([this](Connecting& connection,
                    const std::shared_ptr<RpcMessage>& rpc_message,
                    TimePoint time_point) {
        this->OnRpcMessage(connection, rpc_message, time_point);
      }),
      connection_(nullptr),
      services_(nullptr) {
  LOG_INFO("RpcChannel::RpcChannel - %p", this);
}
RpcChannel::RpcChannel(Connecting& connection)
    : codec_([this](Connecting& connection,
                    const std::shared_ptr<RpcMessage>& rpc_message,
                    TimePoint time_point) {
        this->OnRpcMessage(connection, rpc_message, time_point);
      }),
      connection_(const_cast<Connecting*>(&connection)),
      services_(nullptr) {
  LOG_INFO("RpcChannel::RpcChannel - %p", this);
}
RpcChannel::~RpcChannel() {
  LOG_INFO("RpcChannel::~RpcChannel - %p", this);
  for (auto itr = outstanding_calls_.begin(); itr != outstanding_calls_.end();
       ++itr) {
    auto outstanding_call = itr->second;
    delete outstanding_call.response_message_;
    delete outstanding_call.DoneCallback_;
  }
}

void RpcChannel::CallMethod(
    const ::google::protobuf::MethodDescriptor* method_descriptor,
    ::google::protobuf::RpcController* rpc_controller,
    const ::google::protobuf::Message* request_message,
    ::google::protobuf::Message* response_message,
    ::google::protobuf::Closure* DoneCallback) {
  RpcMessage rpc_message;
  rpc_message.set_type(REQUEST);
  int64_t id = id_.fetch_add(1) + 1;
  rpc_message.set_id(id);
  rpc_message.set_service(method_descriptor->service()->name());
  rpc_message.set_method(method_descriptor->name());
  rpc_message.set_request(request_message->SerializeAsString());
  OutstandingCall outstanding_call = {response_message, DoneCallback};
  {
    LockGuard lock_guard(outstanding_calls_lock_);
    outstanding_calls_[id] = outstanding_call;
  }
  codec_.Send(*connection_, rpc_message);
}

void RpcChannel::OnMessage(Connecting& connection, IoBuffer* io_buffer,
                           TimePoint receive_time) {
  codec_.OnMessage(connection, io_buffer, receive_time);
}

void RpcChannel::OnRpcMessage(
    Connecting& connection, const std::shared_ptr<RpcMessage>& rpc_message_ptr,
    TimePoint receive_time) {
  RpcMessage& rpc_message = *rpc_message_ptr;
  if (rpc_message.type() == RESPONSE) {
    int64_t id = rpc_message.id();
    OutstandingCall outstanding_call = {nullptr, nullptr};
    {
      LockGuard lock_guard(outstanding_calls_lock_);
      auto itr = outstanding_calls_.find(id);
      if (itr != outstanding_calls_.end()) {  // Find the id
        outstanding_call = itr->second;
        outstanding_calls_.erase(itr);
      }
    }
    if (outstanding_call.response_message_ != nullptr) {
      std::unique_ptr<google::protobuf::Message> response_message_ptr(
          outstanding_call.response_message_);
      if (rpc_message.has_response()) {
        outstanding_call.response_message_->ParseFromString(
            rpc_message.response());
      }
      if (outstanding_call.DoneCallback_ != nullptr) {
        outstanding_call.DoneCallback_->Run();
      }
    }
  } else if (rpc_message.type() == REQUEST) {
    ErrorCode error_code = WRONG_PROTO;
    if (services_ != nullptr) {
      auto itr = services_->find(rpc_message.service());
      if (itr != services_->end()) {
        auto service = itr->second;
        const auto* service_descriptor = service->GetDescriptor();
        const auto* method_descriptor =
            service_descriptor->FindMethodByName(rpc_message.method());
        if (method_descriptor != nullptr) {
          std::unique_ptr<google::protobuf::Message> request_message_ptr(
              service->GetRequestPrototype(method_descriptor).New());
          if (request_message_ptr->ParseFromString(rpc_message.request())) {
            auto response_message_t =
                service->GetResponsePrototype(method_descriptor).New();
            int64_t id = rpc_message.id();
            service->CallMethod(
                method_descriptor, nullptr, request_message_ptr.get(),
                response_message_t,
                google::protobuf::NewCallback(this, &RpcChannel::DoneCallback,
                                              response_message_t, id));
            error_code = NO_ERROR;
          } else {
            error_code = INVALID_REQUEST;
          }
        } else {
          error_code = NO_METHOD;
        }
      } else {
        error_code = NO_SERVICE;
      }
    } else {
      error_code = NO_SERVICE;
    }
    if (error_code != NO_ERROR) {
      RpcMessage rpc_response;
      rpc_response.set_type(RESPONSE);
      rpc_response.set_id(rpc_message.id());
      rpc_response.set_error(error_code);
      codec_.Send(connection, rpc_response);
    }
  } else if (rpc_message.type() == ERROR) {
  }
}

void RpcChannel::DoneCallback(::google::protobuf::Message* response_message,
                              int64_t id) {
  std::unique_ptr<google::protobuf::Message> response_message_ptr(
      response_message);
  RpcMessage rpc_message;
  rpc_message.set_type(RESPONSE);
  rpc_message.set_id(id);
  rpc_message.set_response(response_message->SerializeAsString());
  codec_.Send(*connection_, rpc_message);
}
