/**
 * @file rpc_codec.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief
 * @date 2023-08-14
 *
 * @copyright Copyright (c) 2023 Sigma711
 *
 */

#include "rpc_codec.h"

#include <google/protobuf/message_lite.h>
#include <google/protobuf/stubs/common.h>
#include <stddef.h>
#include <zlib.h>

#include <cstdint>
#ifdef __MACH__
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#else
#include <endian.h>
#endif

#include <memory>

#include "logger.h"
#include "rpc.pb.h"

using namespace taotu;

namespace {

int ProtobufVersionCheck() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  return 0;
}
int __attribute__((unused)) dummy = ProtobufVersionCheck();

}  // namespace

void RpcCodec::Send(Connecting& connection,
                    const ::google::protobuf::Message& message) {
  IoBuffer io_buffer;
  FillEmptyBuffer(&io_buffer, message);
  connection.Send(&io_buffer);
}

void RpcCodec::OnMessage(Connecting& connection, IoBuffer* io_buffer,
                         TimePoint receive_time) {
  while (io_buffer->GetReadableBytes() >=
         static_cast<uint32_t>(kMinMessageLength + kHeaderLength)) {
    const int32_t len = io_buffer->GetReadableInt32();
    if (len > kMaxMessageLength || len < kMinMessageLength) {
      ErrCallback_(connection, io_buffer, receive_time,
                   ErrorCode::kInvalidLength);
      break;
    } else if (io_buffer->GetReadableBytes() >=
               static_cast<size_t>(kHeaderLength + len)) {
      if (RawCallback_ &&
          !RawCallback_(connection,
                        std::string_view(io_buffer->GetReadablePosition(),
                                         kHeaderLength + len),
                        receive_time)) {
        io_buffer->Refresh(kHeaderLength + len);
        continue;
      }
      std::shared_ptr<::google::protobuf::Message> message(prototype_->New());
      ErrorCode error_code = Parse(
          io_buffer->GetReadablePosition() + kHeaderLength, len, message.get());
      if (error_code == ErrorCode::kNoError) {
        MessageCallback_(connection, message, receive_time);
        io_buffer->Refresh(kHeaderLength + len);
      } else {
        ErrCallback_(connection, io_buffer, receive_time, error_code);
        break;
      }
    } else {
      break;
    }
  }
}

bool RpcCodec::ParseFromBuffer(std::string_view buffer,
                               ::google::protobuf::Message* message) {
  return message->ParseFromArray(buffer.data(), buffer.size());
}
int RpcCodec::Serialize2Buffer(const ::google::protobuf::Message& message,
                               IoBuffer* io_buffer) {
  size_t byte_size = message.ByteSizeLong();
  io_buffer->EnsureWritableSpace(byte_size + kChecksumLength);
  uint8_t* start = reinterpret_cast<uint8_t*>(
      const_cast<char*>(io_buffer->GetWritablePosition()));
  uint8_t* end = message.SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size || byte_size != message.ByteSizeLong()) {
    LOG_ERROR(
        "Protocol message was modified concurrently during serialization. Or "
        "Byte size calculation and serialization were inconsistent (This may "
        "indicate a bug in protocol buffers or it may be caused by concurrent "
        "modification of the message.). This shouldn't be called if all the "
        "sizes are equal.");
    return -1;
  }
  io_buffer->RefreshW(byte_size);
  return byte_size;
}

namespace {
const std::string kNoErrorStr = "NoError";
const std::string kInvalidLengthStr = "InvalidLength";
const std::string kCheckSumErrorStr = "CheckSumError";
const std::string kInvalidNameLenStr = "InvalidNameLen";
const std::string kUnknownMessageTypeStr = "UnknownMessageType";
const std::string kParseErrorStr = "ParseError";
const std::string kUnknownErrorStr = "UnknownError";
}  // namespace

const std::string& RpcCodec::ErrorCode2String(ErrorCode error_code) {
  switch (error_code) {
    case ErrorCode::kNoError:
      return kNoErrorStr;
    case ErrorCode::kInvalidLength:
      return kInvalidLengthStr;
    case ErrorCode::kChecksumError:
      return kCheckSumErrorStr;
    case ErrorCode::kInvalidNameLength:
      return kInvalidNameLenStr;
    case ErrorCode::kUnknownMessageType:
      return kUnknownMessageTypeStr;
    case ErrorCode::kParseError:
      return kParseErrorStr;
    default:
      return kUnknownErrorStr;
  }
}

RpcCodec::ErrorCode RpcCodec::Parse(const char* buffer, int length,
                                    ::google::protobuf::Message* message) {
  ErrorCode error_code = ErrorCode::kNoError;
  if (ValidateChecksum(buffer, length)) {
    if (::memcmp(reinterpret_cast<const void*>(buffer), tag_.data(),
                 tag_.size()) == 0) {  // Check the tag (the message type)
      const char* data = buffer + tag_.size();
      int32_t data_length =
          length - kChecksumLength - static_cast<int32_t>(tag_.size());
      if (!ParseFromBuffer(std::string_view(data, data_length), message)) {
        error_code = ErrorCode::kParseError;
      }
    } else {
      error_code = ErrorCode::kUnknownMessageType;
    }
  } else {
    error_code = ErrorCode::kChecksumError;
  }
  return error_code;
}
void RpcCodec::FillEmptyBuffer(IoBuffer* io_buffer,
                               const ::google::protobuf::Message& message) {
  io_buffer->Append(static_cast<const void*>(tag_.c_str()), tag_.size());
  int byte_size = Serialize2Buffer(message, io_buffer);
  int32_t checksum = Checksum(io_buffer->GetReadablePosition(),
                              static_cast<int>(io_buffer->GetReadableBytes()));
  io_buffer->AppendInt32(checksum);
  int32_t len = htobe32(static_cast<int32_t>(io_buffer->GetReadableBytes()));
  io_buffer->SetHeadContent(&len, sizeof(len));
}

int32_t RpcCodec::Checksum(const void* buffer, int length) {
  return static_cast<int32_t>(
      ::adler32(1, static_cast<const Bytef*>(buffer), length));
}
bool RpcCodec::ValidateChecksum(const char* buffer, int length) {
  int32_t expected_checksum = AsInt32(buffer + length - kChecksumLength);
  int32_t checksum = Checksum(buffer, length - kChecksumLength);
  return expected_checksum == checksum;
}
int32_t RpcCodec::AsInt32(const char* buffer) {
  int32_t be32 = 0;
  ::memcpy(&be32, buffer, sizeof(be32));
  return be32toh(be32);
}
void RpcCodec::DefaultErrorCallback(Connecting& connection, IoBuffer* io_buffer,
                                    TimePoint time_point,
                                    ErrorCode error_code) {
  LOG_ERROR("RpcCodec::DefaultErrorCallback - %s",
            ErrorCode2String(error_code));
  if (connection.IsConnected()) {
    const_cast<Connecting&>(connection).ShutDownWrite();
  }
}
