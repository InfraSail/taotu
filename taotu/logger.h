/**
 * @file logger.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Logger" which is the log recorder of this
 * library but only 3 Marco APIs are open.
 * @date 2021-11-23
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_LOGGER_H_
#define TAOTU_TAOTU_LOGGER_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "non_copyable_movable.h"

namespace taotu {

// Note: Only these APIs are allowed for log!!!
/************************Open Logger APIs***************************/
// Start the unique logger
#define START_LOG(log_file_name) \
  logger::Logger::GetLogger()->StartLogger(log_file_name)

// End the unique logger
#define END_LOG() logger::Logger::GetLogger()->EndLogger()

// The unique API for recording logs
#define LOG(...) logger::Logger::GetLogger()->RecordLogs(__VA_ARGS__)
/********************************************************************/

namespace logger {

// Relevant to Log_level_info_prefix
enum LogLevel {
  kDebug = 0,
  kWarn,
  kError,
};
namespace {

enum {

  // 2GB
  kLogFileMaxByte = 1024 * 1024 * 1024,

  // Should be the Nth power of 2 (for "Disruptor")
  kLogBufferSize = 1024 * 1024 * 16,
};

}  // namespace

/**
 * @brief "Logger" uses a special ring buffer called "Disruptor": the index of
 * writable position is atomic. And it uses "Singleton" pattern, so there is
 * only one actual "Logger" object in the global environment of one process.
 *
 */
class Logger : NonCopyableMovable {
 public:
  typedef std::shared_ptr<Logger> LoggerPtr;
  typedef std::unique_ptr<std::thread> ThreadPtr;
  typedef std::array<std::string, kLogBufferSize> LogBuffer;

  // The unique method to creat the unique actual "Logger" object ("Singleton"
  // pattern)
  static LoggerPtr GetLogger();
  static void DestroyLogger(Logger* logger);

  void EndLogger();

  // Initialize this logger (have to be called before recording logs)
  void StartLogger(const std::string& log_file_name);
  void StartLogger(std::string&& log_file_name);

  void RecordLogs(LogLevel log_type, const char* log_info);
  void RecordLogs(LogLevel log_type, const std::string& log_info);
  void RecordLogs(LogLevel log_type, std::string&& log_info);

 protected:
  Logger();
  ~Logger() {}

 private:
  void UpdateLoggerTime();

  // Called to write down logs
  void WriteDownLogs();

  void RecordLogs(const char* log_info);
  void RecordLogs(const std::string& log_info);

  // The Actual log-record method (uses "move" semantics)
  void RecordLogs(std::string&& log_info);

  static LoggerPtr logger_ptr;
  static bool is_initialized;

  static constexpr int64_t kStandardLogFileByte = kLogFileMaxByte / 2;

  alignas(256) volatile int64_t is_stopping_;
  alignas(256) char filler1_;  // Only for solving "False Sharing"

  std::mutex log_mutex_;
  std::condition_variable log_cond_var_;

  // Index which was read last time
  alignas(256) volatile int64_t read_index_;
  alignas(256) char filler2_;  // Only for solving "False Sharing"

  int64_t cur_log_file_byte_;
  int64_t cur_log_file_seq_;
  std::string log_file_name_;

  // Index which was written last time
  alignas(256) volatile int64_t wrote_index_;
  alignas(256) char filler3_;  // Only for solving "False Sharing"

  ::FILE* log_file_;

  ThreadPtr thread_;

  std::mutex time_mutex_;
  std::string time_now_str_;
  time_t time_now_sec_;

  // Index which can be written next time
  alignas(256) volatile std::atomic_int64_t write_index_;
  alignas(256) char filler4_;  // Only for solving "False Sharing"

  // A lock-free ring buffer of log-msg ("Disruptor")
  LogBuffer log_buffer_;
};
}  // namespace logger
}  // namespace taotu

#endif  // !TAOTU_TAOTU_LOGGER_H_
