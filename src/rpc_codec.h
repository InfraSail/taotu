/**
 * @file rpc_codec.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief
 * @date 2023-08-14
 *
 * @copyright Copyright (c) 2023 Sigma711
 *
 */

#ifndef TAOTU_SRC_RPC_CODEC_H_
#define TAOTU_SRC_RPC_CODEC_H_

#include <stdint.h>

#include <functional>
#include <string>
#include <string_view>

#include "connecting.h"
#include "io_buffer.h"
#include "non_copyable_movable.h"
#include "rpc.pb.h"
#include "time_point.h"

namespace google {

namespace protobuf {

class Message;

}  // namespace protobuf

}  // namespace google

namespace taotu {

// RPC message format:
// ----------------------------------------------
// |  Field   | Length |         Content        |
// |   size   | 4-byte |          M+N+4         |
// |   tag    | M-byte | could be "RPC0", etc.  |
// | payload  | N-byte |                        |
// | checksum | 4-byte | adler32 of tag+payload |
// ----------------------------------------------

/**
 * @brief "RpcCodec" is a RPC message codec. You should not use it directly but
 * use "RpcCodecT" instead.
 *
 */
class RpcCodec : NonCopyableMovable {
 public:
  static const int kHeaderLength = sizeof(int32_t);
  static const int kChecksumLength = sizeof(int32_t);
  static constexpr int kMaxMessageLength = 64 * 1024 * 1024;

  enum class ErrorCode {
    kNoError = 0,
    kInvalidLength,
    kChecksumError,
    kInvalidNameLength,
    kUnknownMessageType,
    kParseError,
  };

  typedef std::function<void(
      Connecting&, const std::shared_ptr<::google::protobuf::Message>&,
      TimePoint)>
      ProtobufMessageCallback;
  typedef std::function<bool(Connecting&, std::string_view, TimePoint)>
      RawMessageCallback;
  typedef std::function<void(Connecting&, IoBuffer*, TimePoint, ErrorCode)>
      ErrorCallback;

  RpcCodec(const ::google::protobuf::Message* prototype,
           std::string_view tag_arg,
           const ProtobufMessageCallback& MessageCallback,
           const RawMessageCallback& RawCallback = RawMessageCallback{},
           const ErrorCallback& ErrCallback = DefaultErrorCallback)
      : prototype_(prototype),
        tag_(tag_arg),
        kMinMessageLength(tag_arg.size() + kChecksumLength),
        MessageCallback_(MessageCallback),
        RawCallback_(RawCallback),
        ErrCallback_(ErrCallback) {}

  virtual ~RpcCodec() = default;

  const std::string& GetTag() const { return tag_; }

  void Send(Connecting& connection, const ::google::protobuf::Message& message);

  void OnMessage(Connecting& connection, IoBuffer* io_buffer,
                 TimePoint receive_time);

  virtual bool ParseFromBuffer(std::string_view buffer,
                               ::google::protobuf::Message* message);
  virtual int Serialize2Buffer(const ::google::protobuf::Message& message,
                               IoBuffer* io_buffer);

  static const std::string& ErrorCode2String(ErrorCode error_code);

  ErrorCode Parse(const char* buffer, int length,
                  ::google::protobuf::Message* message);
  void FillEmptyBuffer(IoBuffer* io_buffer,
                       const ::google::protobuf::Message& message);

  static int32_t Checksum(const void* buffer, int length);
  static bool ValidateChecksum(const char* buffer, int length);
  static int32_t AsInt32(const char* buffer);
  static void DefaultErrorCallback(Connecting& connection, IoBuffer* io_buffer,
                                   TimePoint time_point, ErrorCode error_code);

 private:
  const ::google::protobuf::Message* prototype_;
  const std::string tag_;
  const int kMinMessageLength;

  ProtobufMessageCallback MessageCallback_;
  RawMessageCallback RawCallback_;
  ErrorCallback ErrCallback_;
};

/**
 * @brief "RpcCodecT" is a RPC message codec with the template version. You
 * should use it instead of "RpcCodec".
 *
 */
template <typename MSG, const char* TAG, typename CODEC = RpcCodec>
class RpcCodecT {
 public:
  typedef std::function<void(Connecting&, const std::shared_ptr<MSG>&,
                             TimePoint)>
      ProtobufMessageCallback;
  typedef RpcCodec::RawMessageCallback RawMessageCallback;
  typedef RpcCodec::ErrorCallback ErrorCallback;

  explicit RpcCodecT(
      const ProtobufMessageCallback& MessageCallback,
      const RawMessageCallback& RawCallback = RawMessageCallback{},
      const ErrorCallback& ErrCallback = RpcCodec::DefaultErrorCallback)
      : MessageCallback_(MessageCallback),
        codec_(
            &MSG::default_instance(), TAG,
            [this](Connecting& connection,
                   const std::shared_ptr<::google::protobuf::Message>& message,
                   TimePoint receive_time) {
              this->OnRpcMessage(connection, message, receive_time);
            },
            RawCallback, ErrCallback) {}

  const std::string& GetTag() const { return codec_.GetTag(); }

  void Send(Connecting& connection, const MSG& message) {
    codec_.Send(connection, message);
  }

  void OnMessage(Connecting& connection, IoBuffer* io_buffer,
                 TimePoint receive_time) {
    codec_.OnMessage(connection, io_buffer, receive_time);
  }

  void OnRpcMessage(Connecting& connection,
                    const std::shared_ptr<::google::protobuf::Message>& message,
                    TimePoint receive_time) {
    MessageCallback_(connection, std::dynamic_pointer_cast<MSG>(message),
                     receive_time);
  }

  void FillEmptyBuffer(IoBuffer* io_buffer, const MSG& message) {
    codec_.FillEmptyBuffer(io_buffer, message);
  }

 private:
  ProtobufMessageCallback MessageCallback_;
  RpcCodec codec_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_RPC_CODEC_H_
