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

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "logger.h"

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

  size_t GetBufferSize() const { return buffer_.size(); }
  size_t GetBufferCapacity() const { return buffer_.capacity(); }

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

  void RefreshRW();
  void RefreshW(size_t len) { writing_index_ += len; }
  void Refresh(size_t len);
  template <class Int>
  void RefreshInt() {
    Refresh(sizeof(Int));
  }

  std::string RetrieveAString(size_t len);
  std::string RetrieveAllAsString() {
    return RetrieveAString(GetReadableBytes());
  }
  template <class Int>
  Int RetrieveInt() {
    Int result = GetReadableInt<Int>();
    RefreshInt<Int>();
    return result;
  }

  void SetHeadContent(const void* str, size_t len);
  template <class Int>
  void SetHeadContentInt(Int x) {
    Int int_str = static_cast<Int>(::htonl(static_cast<uint32_t>(x)));
    SetHeadContent(static_cast<const void*>(&int_str), sizeof(Int));
  }

  void Append(const void* str, size_t len);
  template <class Int>
  void AppendInt(Int x) {
    Int int_str = static_cast<Int>(::htonl(static_cast<uint32_t>(x)));
    Append(static_cast<const void*>(&int_str), sizeof(Int));
  }

  void EnsureWritableSpace(size_t len);

  void ShrinkWritableSpace(size_t len);

  ssize_t ReadFd(int fd, int* tmp_errno);

 private:
  const char* GetBufferBegin() const { return &*buffer_.begin(); }

  void ReserveWritableSpace(size_t len);

  template <class Int>
  Int GetReadableInt() const {
    if (sizeof(Int) > GetReadableBytes()) {
      LOG(logger::kError,
          "Reading the Integer number in Head content failed!!!");
      return static_cast<Int>(-111);
    }
    Int result = static_cast<Int>(0);
    ::memcpy(static_cast<void*>(result), GetBufferBegin(), sizeof(Int));
    return static_cast<Int>(::ntohl(static_cast<uint32_t>(result)));
  }

  std::vector<char> buffer_;
  size_t reading_index_;
  size_t writing_index_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_IO_BUFFER_H_
