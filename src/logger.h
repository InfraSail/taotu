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

#ifndef TAOTU_SRC_LOGGER_H_
#define TAOTU_SRC_LOGGER_H_

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
  logger::Logger::GetLogger(false)->StartLogger(log_file_name)

// End the unique logger
#define END_LOG() logger::Logger::GetLogger(true)->EndLogger()

// The unique API for recording logs
#define LOG(...) logger::Logger::GetLogger(true)->RecordLogs(__VA_ARGS__)

#ifdef TAOTU_DEBUG  // Flag for debug build, set in CMakeLists.txt
#define LOG_DEBUG(...) LOG(taotu::logger::kDebug, __VA_ARGS__)
#else  // release build
#define LOG_DEBUG(...) TrivialFunc()
#endif  // TAOTU_DEBUG

#define LOG_INFO(...) LOG(taotu::logger::kInfo, __VA_ARGS__)
#define LOG_NOTICE(...) LOG(taotu::logger::kNotice, __VA_ARGS__)
#define LOG_WARN(...) LOG(taotu::logger::kWarn, __VA_ARGS__)
#define LOG_ERROR(...) LOG(taotu::logger::kError, __VA_ARGS__)
#define LOG_CRIT(...) LOG(taotu::logger::kCrit, __VA_ARGS__)
#define LOG_ALERT(...) LOG(taotu::logger::kAlert, __VA_ARGS__)
#define LOG_EMERG(...) LOG(taotu::logger::kEmerg, __VA_ARGS__)

/********************************************************************/

inline void TrivialFunc() {}

namespace logger {

// Relevant to Log_level_info_prefix
// Levels are taken from <syslog.h>
enum LogLevel {
  kEmerg = 0,
  kAlert,
  kCrit,
  kError,
  kWarn,
  kNotice,
  kInfo,
  kDebug,
};

constexpr int64_t kLogFileMaxByte = 1024 * 1024 * 1024;  // 2GB
constexpr size_t kLogBufferSize = 1024 * 1024 * 16;      // Power of two.

// The file name of the log
static const std::string kLogName{"log.txt"};

// Relevant to LogLevel
static const std::string Log_level_info_prefix[]{
    "Log(Emergency): ", "Log(Alert): ",  "Log(Critical): ", "Log(Error): ",
    "Log(Warn): ",      "Log(Notice): ", "Log(Info): ",     "Log(Debug): ",
};

/**
 * @brief "Logger" uses a special ring buffer called "Disruptor": the index of
 * writable position is atomic. And it uses "Singleton" pattern, so there is
 * only one actual "Logger" object in the global environment of one process.
 *
 */
class Logger : NonCopyableMovable {
 public:
  struct LogSlot {
    std::atomic<size_t> seq{0};
    std::string data;
  };
  typedef std::array<LogSlot, kLogBufferSize> LogBuffer;

  // The unique method to creat the unique actual "Logger" object ("Singleton"
  // pattern)
  static Logger* GetLogger(bool should_start);

  void EndLogger();

  // Initialize this logger (have to be called before recording logs)
  void StartLogger(const std::string& log_file_name);

  // Initialize this logger (have to be called before recording logs)
  void StartLogger(std::string&& log_file_name);

  // Record log (use variable length parameters)
  template <class... Args>
  void RecordLogs(LogLevel log_type, const char* log_info, Args... args) {
    int msg_len =
        ::snprintf(nullptr, static_cast<size_t>(0), log_info, args...);
    size_t prefix_size = Log_level_info_prefix[log_type].size();
    char* message = new char[prefix_size + static_cast<size_t>(msg_len) + 1];
    ::memcpy(static_cast<void*>(message),
             Log_level_info_prefix[log_type].c_str(), prefix_size);
    ::snprintf(message + prefix_size, static_cast<size_t>(msg_len + 1),
               log_info, args...);
    RecordLogs(static_cast<const char*>(message));
    delete[] message;
  }

  // Record log
  void RecordLogs(LogLevel log_type, const std::string& log_info);

  // Record log
  void RecordLogs(LogLevel log_type, std::string&& log_info);

 protected:
  Logger();
  ~Logger();

 private:
  std::string UpdateLoggerTime();

  // Called to write down logs
  void WriteDownLogs();

  bool Enqueue(std::string&& log_data);
  bool Dequeue(std::string* out);

  void RecordLogs(const char* log_info);
  void RecordLogs(const std::string& log_info);

  // The Actual log-record method (uses "move" semantics)
  void RecordLogs(std::string&& log_info);

  static std::atomic<bool> is_initialized;

  static constexpr int64_t kStandardLogFileByte = kLogFileMaxByte / 2;
  static constexpr size_t kLogBufferMask =
      static_cast<size_t>(kLogBufferSize - 1);

  alignas(256) std::atomic<bool> is_stopping_;
  alignas(256) char filler1_;  // Only for solving "False Sharing"

  std::mutex log_mutex_;
  std::condition_variable log_cond_var_;

  int64_t cur_log_file_byte_;
  int64_t cur_log_file_seq_;
  std::string log_file_name_;

  FILE* log_file_;

  std::thread thread_;

  std::mutex time_mutex_;
  std::string time_now_str_;
  time_t time_now_sec_;

  // Queue indices
  alignas(256) std::atomic<size_t> write_index_;
  alignas(256) char filler2_;  // Only for solving "False Sharing"
  alignas(256) std::atomic<size_t> read_index_;
  alignas(256) char filler3_;  // Only for solving "False Sharing"
  alignas(256) std::atomic<size_t> pending_;
  alignas(256) char filler4_;  // Only for solving "False Sharing"

  // A lock-free ring buffer of log-msg ("Disruptor")
  LogBuffer log_buffer_;
};

}  // namespace logger

}  // namespace taotu

#endif  // !TAOTU_SRC_LOGGER_H_
