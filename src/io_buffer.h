/**
 * @file io_buffer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "IoBuffer" which is the buffer of I/O.
 * @date 2021-12-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_IO_BUFFER_H_
#define TAOTU_SRC_IO_BUFFER_H_

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

}  // namespace

#if defined(__clang__) || __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

/**
 * @brief "IoBuffer" uses multiple contiguous and scalable memory blocks as the
 * I/O buffer. It records the reading and writing indexes and always reserves
 * more than 8 bytes in the front of the buffer as an optional message header.
 * For Coping with large traffic, it provides a solution using discrete reading.
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

  // Reset writing index after writing by a specific length
  void RefreshW(size_t len) { writing_index_ += len; }

  // Reset indexes by a specific length
  void Refresh(size_t len);

  // Read content as a string that has a specific length
  std::string RetrieveAString(size_t len);

  // Read all content as a string
  std::string RetrieveAllAsString() {
    return RetrieveAString(GetReadableBytes());
  }

  // Read content as an 8-bit integer (refresh the indexes)
  int8_t RetrieveInt8();

  // Read content as a 16-bit integer (refresh the indexes)
  int16_t RetrieveInt16();

  // Read content as a 32-bit integer (refresh the indexes)
  int32_t RetrieveInt32();

  // Read content as a 64-bit integer (refresh the indexes)
  int64_t RetrieveInt64();

  // Get a readable 8-bit integer (do not refresh the indexes)
  int8_t GetReadableInt8() const;

  // Get a readable 16-bit integer (do not refresh the indexes)
  int16_t GetReadableInt16() const;

  // Get a readable 32-bit integer (do not refresh the indexes)
  int32_t GetReadableInt32() const;

  // Get a readable 64-bit integer (do not refresh the indexes)
  int64_t GetReadableInt64() const;

  // Set the optional message header which is a length-limited string
  void SetHeadContent(const void* str, size_t len);

  // Set the optional message header which is an 8-bit integer
  void SetHeadContentInt8(int8_t x);

  // Set the optional message header which is a 16-bit integer
  void SetHeadContentInt16(int16_t x);

  // Set the optional message header which is a 32-bit integer
  void SetHeadContentInt32(int32_t x);

  // Set the optional message header which is a 64-bit integer
  void SetHeadContentInt64(int64_t x);

  // Write content which is a string
  void Append(const void* str, size_t len);

  // Write content which is an 8-bit integer
  void AppendInt8(int8_t x);

  // Write content which is a 16-bit integer
  void AppendInt16(int16_t x);

  // Write content which is a 32-bit integer
  void AppendInt32(int32_t x);

  // Write content which is a 64-bit integer
  void AppendInt64(int64_t x);

  // Ensure that there is enough space for writing
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

  // Get the raw begin of the buffer
  const char* GetBufferBegin() const { return &*buffer_.begin(); }

  // Reserve space for writing
  void ReserveWritableSpace(size_t len);

  BufferType buffer_;
  size_t reading_index_;
  size_t writing_index_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_IO_BUFFER_H_
