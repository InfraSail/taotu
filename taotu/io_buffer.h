/**
 * @file io_buffer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_IO_BUFFER_H_
#define TAOTU_TAOTU_IO_BUFFER_H_

#include <stdlib.h>

#include <vector>

namespace taotu {

namespace {
enum {
  kReservedCapacity = 8,
  kInitialCapacity = 1024,
};
static const char kCrlf[] = "\r\n";
}  // namespace

/**
 * @brief  // TODO:
 *
 */
class IoBuffer {
 public:
  explicit IoBuffer(size_t initial_capacity = kInitialCapacity);

  void Swap(IoBuffer& io_buffer);

  size_t GetReadableBytes() const { return writing_index_ - reading_index_; }
  size_t GetWritableBytes() const { return buffer_.size() - writing_index_; }
  size_t GetReservedBytes() const { return reading_index_; }

  const char* GetReadablePosition() const {
    return GetBufferBegin() + reading_index_;
  }
  const char* GetWritablePosition() const {
    return GetBufferBegin() + writing_index_;
  }

  const char* FindCrlf() const;
  const char* FindCrlf(const char* start_position) const;

  const char* FindEof() const;
  const char* FindEof(const char* start_position) const;

 private:
  const char* GetBufferBegin() const { return &*buffer_.begin(); }

  std::vector<char> buffer_;
  size_t reading_index_;
  size_t writing_index_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_IO_BUFFER_H_
