/**
 * @file io_buffer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "IoBuffer" which is the buffer of I/O.
 * @date 2021-12-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "io_buffer.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <utility>

#include "logger.h"

namespace taotu {

namespace {
constexpr char kCrlf[] = "\r\n";
}  // namespace

IoBuffer::IoBuffer(size_t initial_capacity)
    : buffer_(kReservedCapacity + initial_capacity),
      reading_index_(kReservedCapacity),
      writing_index_(kReservedCapacity) {}

void IoBuffer::Swap(IoBuffer& io_buffer) {
  buffer_.swap(io_buffer.buffer_);
  std::swap(reading_index_, io_buffer.reading_index_);
  std::swap(writing_index_, io_buffer.writing_index_);
}

const char* IoBuffer::FindCrlf() const {
  return static_cast<const char*>(::memmem(GetReadablePosition(),
                                           GetReadableBytes(), kCrlf,
                                           static_cast<size_t>(2)));
}

const char* IoBuffer::FindCrlf(const char* start_position) const {
  return static_cast<const char*>(
      ::memmem(start_position, GetWritablePosition() - start_position, kCrlf,
               static_cast<size_t>(2)));
}

const char* IoBuffer::FindEof() const {
  return static_cast<const char*>(
      ::memchr(GetReadablePosition(), '\n', GetReadableBytes()));
}
const char* IoBuffer::FindEof(const char* start_position) const {
  return static_cast<const char*>(
      ::memchr(start_position, '\n', GetWritablePosition() - start_position));
}

void IoBuffer::RefreshRW() {
  reading_index_ = kReservedCapacity;
  writing_index_ = kReservedCapacity;
}
void IoBuffer::Refresh(size_t len) {
  if (len < GetReadableBytes()) {
    reading_index_ += len;
  } else {  // len == GetReadableBytes()
    RefreshRW();
  }
}

std::string IoBuffer::RetrieveAString(size_t len) {
  if (len > GetReadableBytes()) {
    LOG_ERROR("Read too many bytes from the buffer!!!");
    return std::string{};
  }
  std::string ret(GetReadablePosition(), len);
  Refresh(len);
  return ret;
}

std::vector<char> IoBuffer::RetrieveAByteArray(size_t len) {
  if (len > GetReadableBytes()) {
    LOG_ERROR("Read too many bytes from the buffer!!!");
    return std::vector<char>{};
  }
  std::vector<char> ret(len);
  std::copy(GetReadablePosition(), GetReadablePosition() + len,
            &(*ret.begin()));
  Refresh(len);
  return ret;
}

int8_t IoBuffer::RetrieveInt8() {
  auto result = GetReadableInt8();
  Refresh(sizeof(int8_t));
  return result;
}

int16_t IoBuffer::RetrieveInt16() {
  auto result = GetReadableInt16();
  Refresh(sizeof(int16_t));
  return result;
}

int32_t IoBuffer::RetrieveInt32() {
  auto result = GetReadableInt32();
  Refresh(sizeof(int32_t));
  return result;
}

int64_t IoBuffer::RetrieveInt64() {
  auto result = GetReadableInt64();
  Refresh(sizeof(int64_t));
  return result;
}

int8_t IoBuffer::GetReadableInt8() const {
  auto result = static_cast<int8_t>(0);
  if (sizeof(int8_t) > GetReadableBytes()) {
    LOG_ERROR("Reading the Integer number in Head content failed!!!");
    return result;
  }
  result = static_cast<int8_t>(*GetReadablePosition());
  return result;
}

int16_t IoBuffer::GetReadableInt16() const {
  auto result = static_cast<int16_t>(0);
  if (sizeof(int16_t) > GetReadableBytes()) {
    LOG_ERROR("Reading the Integer number in Head content failed!!!");
    return result;
  }
  ::memcpy(static_cast<void*>(&result), GetReadablePosition(), sizeof(int16_t));
  return static_cast<int16_t>(be16toh(static_cast<uint16_t>(result)));
}

int32_t IoBuffer::GetReadableInt32() const {
  auto result = static_cast<int32_t>(0);
  if (sizeof(int32_t) > GetReadableBytes()) {
    LOG_ERROR("Reading the Integer number in Head content failed!!!");
    return result;
  }
  ::memcpy(static_cast<void*>(&result), GetReadablePosition(), sizeof(int32_t));
  return static_cast<int32_t>(be32toh(static_cast<uint32_t>(result)));
}

int64_t IoBuffer::GetReadableInt64() const {
  auto result = static_cast<int64_t>(0);
  if (sizeof(int64_t) > GetReadableBytes()) {
    LOG_ERROR("Reading the Integer number in Head content failed!!!");
    return result;
  }
  ::memcpy(static_cast<void*>(&result), GetReadablePosition(), sizeof(int64_t));
  return static_cast<int64_t>(be64toh(static_cast<uint64_t>(result)));
}

void IoBuffer::SetHeadContent(const void* str, size_t len) {
  if (len > GetReservedBytes()) {
    LOG_ERROR("Reserved head space is not enough!!!");
    return;
  }
  reading_index_ -= len;
  ::memcpy(static_cast<void*>(const_cast<char*>(GetReadablePosition())), str,
           len);
}

void IoBuffer::SetHeadContentInt8(int8_t x) {
  SetHeadContent(static_cast<const void*>(&x), sizeof(x));
}

void IoBuffer::SetHeadContentInt16(int16_t x) {
  auto int_bt = static_cast<int16_t>(htobe16(static_cast<uint16_t>(x)));
  SetHeadContent(static_cast<const void*>(&int_bt), sizeof(int_bt));
}

void IoBuffer::SetHeadContentInt32(int32_t x) {
  auto int_bt = static_cast<int32_t>(htobe32(static_cast<uint32_t>(x)));
  SetHeadContent(static_cast<const void*>(&int_bt), sizeof(int_bt));
}

void IoBuffer::SetHeadContentInt64(int64_t x) {
  auto int_bt = static_cast<int64_t>(htobe64(static_cast<uint64_t>(x)));
  SetHeadContent(static_cast<const void*>(&int_bt), sizeof(int_bt));
}

void IoBuffer::Append(const void* str, size_t len) {
  EnsureWritableSpace(len);
  ::memcpy(static_cast<void*>(const_cast<char*>(GetWritablePosition())), str,
           len);
  RefreshW(len);
}

void IoBuffer::AppendInt8(int8_t x) {
  Append(static_cast<const void*>(&x), sizeof(x));
}

void IoBuffer::AppendInt16(int16_t x) {
  auto int_bt = static_cast<int16_t>(htobe16(static_cast<uint16_t>(x)));
  Append(static_cast<const void*>(&int_bt), sizeof(int_bt));
}

void IoBuffer::AppendInt32(int32_t x) {
  auto int_bt = static_cast<int32_t>(htobe32(static_cast<uint32_t>(x)));
  Append(static_cast<const void*>(&int_bt), sizeof(int_bt));
}

void IoBuffer::AppendInt64(int64_t x) {
  auto int_bt = static_cast<int64_t>(htobe64(static_cast<uint64_t>(x)));
  Append(static_cast<const void*>(&int_bt), sizeof(int_bt));
}

void IoBuffer::EnsureWritableSpace(size_t len) {
  if (len > GetWritableBytes()) {
    ReserveWritableSpace(len);
  }
}

void IoBuffer::ShrinkWritableSpace(size_t len) {
  // We do not use the member function of vector<> -- shrink_to_fit() because it
  // is a non-binding request
  if (len <= 32) {  // Reserving too little writable space is meaningless
    LOG_WARN("Shrinking buffer to %ubytes failed!", len);
    return;
  }
  IoBuffer buffer;
  buffer.EnsureWritableSpace(kReservedCapacity + GetReadableBytes() + len);
  buffer.Append(static_cast<const void*>(GetReadablePosition()),
                GetReadableBytes());
  Swap(buffer);
}

ssize_t IoBuffer::ReadFromFd(int fd, int* tmp_errno) {
  char extra_buffer[64 * 1024];  // 64k bytes
  struct iovec discrete_buffers[2];
  int writable_bytes = GetWritableBytes();
  discrete_buffers[0].iov_base =
      static_cast<void*>(const_cast<char*>(GetWritablePosition()));
  discrete_buffers[0].iov_len = writable_bytes;
  discrete_buffers[1].iov_base = static_cast<void*>(extra_buffer);
  discrete_buffers[1].iov_len = 64 * 1024;
  // Use extra buffer to receive data if the writable space is not enough
  const int iov_seq = writable_bytes < 64 * 1024 ? 2 : 1;
  // To replace this (because of signals):
  // ssize_t n = ::readv(fd, static_cast<const struct iovec*>(discrete_buffers),
  // iov_seq);
  struct msghdr message;
  ::memset(&message, 0, sizeof(message));  // clear the structure
  message.msg_iov = discrete_buffers;
  message.msg_iovlen = iov_seq;
  ssize_t n = ::recvmsg(fd, &message, MSG_NOSIGNAL);
  if (n < 0) {
    *tmp_errno = errno;
    // EAGAIN/EWOULDBLOCK/EINTR are ignorable, non-fatal conditions.
    if (*tmp_errno != EAGAIN && *tmp_errno != EWOULDBLOCK &&
        *tmp_errno != EINTR) {
      char errbuf[128];
      errbuf[0] = '\0';
      (void)::strerror_r(*tmp_errno, errbuf, sizeof(errbuf));
      LOG_ERROR("Discrete reading in Fd(%d) failed!!! errno(%d): %s", fd,
                *tmp_errno, errbuf);
    }
  } else if (static_cast<size_t>(n) <= static_cast<size_t>(writable_bytes)) {
    writing_index_ += n;
  } else {
    writing_index_ = buffer_.size();
    Append(static_cast<const void*>(extra_buffer),
           static_cast<size_t>(n - writable_bytes));
  }
  return n;
}
ssize_t IoBuffer::ReadFromFd(int fd, size_t read_len, int* tmp_errno) {
  EnsureWritableSpace(read_len);
  ssize_t res = 0;
  while (read_len > 0) {
    auto bytes_read =
        ::recv(fd, static_cast<void*>(const_cast<char*>(GetWritablePosition())),
               read_len, MSG_NOSIGNAL);
    read_len -= bytes_read;
    writing_index_ += bytes_read;
    res += bytes_read;
  }
  return res;
}

ssize_t IoBuffer::WriteToFd(int fd) {
  ssize_t n = ::send(
      fd, reinterpret_cast<void*>(const_cast<char*>(GetReadablePosition())),
      static_cast<size_t>(GetReadableBytes()), MSG_NOSIGNAL);
  if (n > 0) {
    Refresh(static_cast<size_t>(n));
  }
  return n;
}

void IoBuffer::ReserveWritableSpace(size_t len) {
  if (GetWritableBytes() + GetReservedBytes() - kReservedCapacity < len) {
    buffer_.resize(writing_index_ + len);
  } else {
    // Move forward to-read contents if too much space are reserved in the
    // front of the buffer, and then the writable space will be enough without
    // dilatation
    ::memmove(static_cast<void*>(
                  const_cast<char*>(GetBufferBegin() + kReservedCapacity)),
              static_cast<const void*>(GetBufferBegin() + reading_index_),
              GetReadableBytes());
    reading_index_ = kReservedCapacity;
    writing_index_ = reading_index_ + GetReadableBytes();
  }
}

}  // namespace taotu
