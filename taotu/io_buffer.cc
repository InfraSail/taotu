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

#include <algorithm>

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

std::string IoBuffer::ReadAString(size_t len) {
  if (len > GetReadableBytes()) {
    LOG(logger::kError, "Read too many bytes from the buffer!!!");
    return std::string{};
  }
  std::string ret(GetReadablePosition(), len);
  Refresh(len);
  return ret;
}
