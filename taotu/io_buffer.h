/**
 * @file io_buffer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "IoBuffer" which is the buffer of I/O.
 * @date 2021-12-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_IO_BUFFER_H_
#define TAOTU_TAOTU_IO_BUFFER_H_

#include <sys/types.h>
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
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "logger.h"

namespace taotu {

namespace {
enum {
  kReservedCapacity = 8,  // vacates 8 bytes in the front of the buffer as an
                          // optional message header
  kInitialCapacity = 1024,
};
static const char kCrlf[] = "\r\n";
}  // namespace

#if defined(__clang__) || __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

/**
 * @brief "IoBuffer" uses multiple contiguous and scalable memory blocks as the
 * I/O buffer. It records the reading and writing indexes and always reserves 8
 * bytes in the front of the buffer as an optional message header. For Coping
 * with large traffic, it provides a solution using discrete reading.
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

  // Reset reading and writing index to initial status
  void RefreshRW();

  // Reset writing index after writing
  void RefreshW(size_t len) { writing_index_ += len; }

  // Reset indexes
  void Refresh(size_t len);

  template <class Int>
  void RefreshInt() {
    Refresh(sizeof(Int));
  }

  // Read content
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

  // Set the optional message header
  void SetHeadContent(const void* str, size_t len);
  template <class Int>
  void SetHeadContentInt(Int x) {
    Int int_str = Host2Network<Int>(x);
    SetHeadContent(static_cast<const void*>(&int_str), sizeof(Int));
  }

  // Write content
  void Append(const void* str, size_t len);
  template <class Int>
  void AppendInt(Int x) {
    Int int_str = Host2Network<Int>(x);
    Append(static_cast<const void*>(&int_str), sizeof(Int));
  }

  void EnsureWritableSpace(size_t len);

  // Only be called in user code
  void ShrinkWritableSpace(size_t len);

  // Retrieve content from the file descriptor with discrete reading(coping with
  // sudden large traffic) to the buffer
  ssize_t ReadFromFd(int fd, int* tmp_errno);

  // Stuff content of the buffer into the file descriptor
  ssize_t WriteToFd(int fd);

 private:
  typedef std::vector<char> BufferType;

  const char* GetBufferBegin() const { return &*buffer_.begin(); }

  // Reserve space for writing
  void ReserveWritableSpace(size_t len);

  template <class Int>
  Int GetReadableInt() const {
    if (sizeof(Int) > GetReadableBytes()) {
      LOG(logger::kError,
          "Reading the Integer number in Head content failed!!!");
      return static_cast<Int>(0);
    }
    Int result = static_cast<Int>(0);
    ::memcpy(static_cast<void*>(result), GetBufferBegin(), sizeof(Int));
    return Network2Host<Int>(result);
  }

  template <class Int>
  static Int Host2Network(Int x) {
    switch (sizeof(x)) {
      case 8:
        return x;
      case 16:
        return static_cast<int16_t>(htobe16(static_cast<uint16_t>(x)));
      case 32:
        return static_cast<int32_t>(htobe32(static_cast<uint32_t>(x)));
      case 64:
        return static_cast<int64_t>(htobe64(static_cast<uint64_t>(x)));
      default:
        return static_cast<Int>(0);
    }
  }

  template <class Int>
  static Int Network2Host(Int x) {
    switch (sizeof(x)) {
      case 8:
        return x;
      case 16:
        return static_cast<int16_t>(be16toh(static_cast<uint16_t>(x)));
      case 32:
        return static_cast<int32_t>(be32toh(static_cast<uint32_t>(x)));
      case 64:
        return static_cast<int64_t>(be64toh(static_cast<uint64_t>(x)));
      default:
        return static_cast<Int>(0);
    }
  }

  BufferType buffer_;
  size_t reading_index_;
  size_t writing_index_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_IO_BUFFER_H_
