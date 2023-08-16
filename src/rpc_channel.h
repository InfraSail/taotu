/**
 * @file rpc_channel.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declarations of class "RpcChannel" which represents the single RPC
 * message communication pipe.
 * @date 2023-07-21
 *
 * @copyright Copyright (c) 2023 Sigma711
 *
 */

#ifndef TAOTU_SRC_RPC_CHANNEL_H_
#define TAOTU_SRC_RPC_CHANNEL_H_

#include <google/protobuf/service.h>

#include <atomic>
#include <memory>
#include <unordered_map>

#include "connecting.h"
#include "io_buffer.h"
#include "rpc.pb.h"
#include "rpc_codec.h"
#include "spin_lock.h"
#include "time_point.h"

namespace google {

namespace protobuf {

class Descriptor;
class ServiceDescriptor;
class MethodDescriptor;
class Message;

class Closure;

class RpcController;
class Service;

}  // namespace protobuf

}  // namespace google

namespace taotu {

extern const char kRpcTag[];

/**
 * @brief "RpcChannel" represents a communication pipe to a RPC service.
 *
 */
class RpcChannel : public ::google::protobuf::RpcChannel {
 public:
  RpcChannel();
  explicit RpcChannel(Connecting& connection);
  ~RpcChannel();

  void SetConnection(Connecting& connection) { connection_ = &connection; }

  void SetServices(
      std::unordered_map<std::string, ::google::protobuf::Service*>* services) {
    services_ = services;
  }

  void CallMethod(const ::google::protobuf::MethodDescriptor* method_descriptor,
                  ::google::protobuf::RpcController* rpc_controller,
                  const ::google::protobuf::Message* request_message,
                  ::google::protobuf::Message* response_message,
                  ::google::protobuf::Closure* DoneCallback);

  void OnMessage(Connecting& connection, IoBuffer* io_buffer,
                 TimePoint receive_time);

 private:
  void OnRpcMessage(Connecting& connection,
                    const std::shared_ptr<RpcMessage>& rpc_message_ptr,
                    TimePoint receive_time);

  void DoneCallback(::google::protobuf::Message* response_message, int64_t id);

  struct OutstandingCall {
    ::google::protobuf::Message* response_message_;
    ::google::protobuf::Closure* DoneCallback_;
  };

  RpcCodecT<RpcMessage, kRpcTag> codec_;
  Connecting* connection_;
  std::atomic_int64_t id_;

  MutexLock outstanding_calls_lock_;
  std::unordered_map<int64_t, OutstandingCall> outstanding_calls_;

  const std::unordered_map<std::string, ::google::protobuf::Service*>*
      services_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_RPC_CHANNEL_H_
