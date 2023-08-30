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
#include "time_point.h"

using namespace taotu;

namespace taotu {

const char kRpcTag[] = "RPC0";

}  // namespace taotu

RpcAsyncChannel::RpcAsyncChannel()
    : codec_([this](Connecting& connection,
                    const std::shared_ptr<RpcMessage>& rpc_message_ptr,
                    TimePoint receive_time) {
        this->OnRpcMessage(connection, rpc_message_ptr, receive_time);
      }),
      connection_(nullptr),
      services_(nullptr) {
  LOG_INFO("RpcAsyncChannel::RpcAsyncChannel - %p", this);
}
RpcAsyncChannel::RpcAsyncChannel(Connecting& connection)
    : codec_([this](Connecting& connection,
                    const std::shared_ptr<RpcMessage>& rpc_message,
                    TimePoint time_point) {
        this->OnRpcMessage(connection, rpc_message, time_point);
      }),
      connection_(const_cast<Connecting*>(&connection)),
      services_(nullptr) {
  LOG_INFO("RpcAsyncChannel::RpcAsyncChannel - %p", this);
}
RpcAsyncChannel::~RpcAsyncChannel() {
  LOG_INFO("RpcAsyncChannel::~RpcAsyncChannel - %p", this);
  for (auto itr = outstanding_calls_.begin(); itr != outstanding_calls_.end();
       ++itr) {
    auto outstanding_call = itr->second;
    if (outstanding_call.response_message_ != nullptr) {
      delete outstanding_call.response_message_;
    }
    if (outstanding_call.DoneCallback_ != nullptr) {
      delete outstanding_call.DoneCallback_;
    }
  }
}

void RpcAsyncChannel::CallMethod(
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

void RpcAsyncChannel::OnMessage(Connecting& connection, IoBuffer* io_buffer,
                                TimePoint receive_time) {
  codec_.OnMessage(connection, io_buffer, receive_time);
}

void RpcAsyncChannel::OnRpcMessage(
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
            service->CallMethod(method_descriptor, nullptr,
                                request_message_ptr.get(), response_message_t,
                                google::protobuf::NewCallback(
                                    this, &RpcAsyncChannel::DoneCallback,
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

void RpcAsyncChannel::DoneCallback(
    ::google::protobuf::Message* response_message, int64_t id) {
  std::unique_ptr<google::protobuf::Message> response_message_ptr(
      response_message);
  RpcMessage rpc_message;
  rpc_message.set_type(RESPONSE);
  rpc_message.set_id(id);
  rpc_message.set_response(response_message->SerializeAsString());
  codec_.Send(*connection_, rpc_message);
}

RpcSyncChannel::RpcSyncChannel(const NetAddress& server_address)
    : server_address_(server_address),
      codec_([this](int sock_fd, const std::shared_ptr<RpcMessage>& rpc_message,
                    TimePoint receive_time) {
        this->OnRpcMessage(sock_fd, rpc_message, receive_time);
      }),
      socket_fd_(::socket(server_address.GetFamily(),
                          SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP)) {
  LOG_INFO("RpcSyncChannel::RpcSyncChannel - %p", this);
  if (socket_fd_ < 0) {
    LOG_ERROR("Fail to initialize for the socket fd of new RpcSyncChannel!!!");
    ::exit(-1);
  }
#ifndef __linux__
  int flags = ::fcntl(socket_fd_, F_GETFL, 0);
  flags |= FD_CLOEXEC;
  ::fcntl(socket_fd_, F_SETFD, flags);
#endif
  if (::connect(socket_fd_, server_address_.GetNetAddress(),
                server_address_.GetSize()) == -1) {
    LOG_ERROR("RpcSyncChannel cannot connect to [ IP(%s), Port(%s) ]!!!",
              server_address_.GetIp().c_str(),
              std::to_string(server_address_.GetPort()).c_str());
    ::close(socket_fd_);
    socket_fd_ = -100;
  }
}
RpcSyncChannel::~RpcSyncChannel() {
  LOG_INFO("RpcSyncChannel::~RpcSyncChannel - %p", this);
  if (socket_fd_ >= 0) {
    ::close(socket_fd_);
  }
  // for (auto itr = outstanding_calls_.begin(); itr !=
  // outstanding_calls_.end();
  //      ++itr) {
  //   auto outstanding_call = itr->second;
  //   if (outstanding_call.response_message_ != nullptr) {
  //     delete outstanding_call.response_message_;
  //   }
  //   if (outstanding_call.DoneCallback_ != nullptr) {
  //     delete outstanding_call.DoneCallback_;
  //   }
  // }
}

void RpcSyncChannel::CallMethod(
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
  if (socket_fd_ < 0) {
    return;
  }
  codec_.Send(socket_fd_, rpc_message);
  if (socket_fd_ < 0) {
    return;
  }
  OnMessage(socket_fd_, &io_buffer_, TimePoint{});
}

void RpcSyncChannel::OnMessage(int sock_fd, IoBuffer* io_buffer,
                               TimePoint receive_time) {
  codec_.OnMessage(sock_fd, io_buffer, receive_time);
}

void RpcSyncChannel::OnRpcMessage(
    int sock_fd, const std::shared_ptr<RpcMessage>& rpc_message_ptr,
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
      if (rpc_message.has_response()) {
        outstanding_call.response_message_->ParseFromString(
            rpc_message.response());
      }
      if (outstanding_call.DoneCallback_ != nullptr) {
        outstanding_call.DoneCallback_->Run();
      }
    }
  } else {
    LOG_ERROR("RpcSyncClient - Rpc message type is not RESPONSE!!!");
  }
}
