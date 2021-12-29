/**
 * @file io_buffer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "io_buffer.h"

#include <string.h>

#include <utility>

#include "logger.h"

using namespace taotu;

IoBuffer::IoBuffer(size_t initial_capacity = kInitialCapacity)
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
    LOG(logger::kError, "Read too many bytes from the buffer!!!");
    return std::string{};
  }
  std::string ret(GetReadablePosition(), len);
  Refresh(len);
  return ret;
}

void IoBuffer::SetHeadContent(const void* str, size_t len) {
  if (len > GetReservedBytes()) {
    LOG(logger::kError, "Reserved head space is not enough!!!");
    return;
  }
  reading_index_ -= len;
  ::memcpy(static_cast<void*>(const_cast<char*>(GetReadablePosition())), str,
           len);
}

void IoBuffer::Append(const void* str, size_t len) {
  EnsureWritableSpace(len);
  ::memcpy(static_cast<void*>(const_cast<char*>(GetWritablePosition())), str,
           len);
  RefreshW(len);
}

void IoBuffer::EnsureWritableSpace(size_t len) {
  if (len > GetWritableBytes()) {
    ReserveWritableSpace(len);
  }
}

void IoBuffer::ShrinkWritableSpace(size_t len) {
  // We do not use the function of vector<> -- shrink_to_fit() because it is a
  // non-binding request
  if (len <= 32) {  // Reserving too little writable space is meaningless
    LOG(logger::kWarn,
        "Shrinking buffer to " + std::to_string(len) + "bytes failed!");
    return;
  }
  IoBuffer buffer;
  buffer.EnsureWritableSpace(kReservedCapacity + GetReadableBytes() + len);
  buffer.Append(static_cast<const void*>(GetReadablePosition()),
                GetReadableBytes());
  Swap(buffer);
}

ssize_t ReadFd(int fd, int* tmp_errno) {}

void IoBuffer::ReserveWritableSpace(size_t len) {
  if (GetWritableBytes() + GetReservedBytes() - kReservedCapacity < len) {
    buffer_.resize(writing_index_ + len);
  } else {
    // Move forward to-read contents if too much space are reserved in the
    // front of the buffer
    // Then the writable space will be enough without dilatating
    ::memcpy(static_cast<void*>(
                 const_cast<char*>(GetBufferBegin() + kReservedCapacity)),
             static_cast<const void*>(GetBufferBegin() + reading_index_),
             GetReadableBytes());
    reading_index_ = kReservedCapacity;
    writing_index_ = reading_index_ + GetReadableBytes();
  }
}
