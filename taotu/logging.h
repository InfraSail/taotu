/**
 * @file logging.h
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief // TODO:
 * @date 2021-11-23
 *
 * @license: MIT
 * @copyright Copyright (c) 2021 Sigma711
 *
 */
#ifndef TAOTU_TAOTU_LOGGING_H_
#define TAOTU_TAOTU_LOGGING_H_

#include <stdio.h>

#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "configurations.h"
#include "non_copyable_movable.h"

namespace taotu {

// Start the unique logger
#define START_LOG(log_file_name) \
  logger::Logger::GetLogger()->StartLogger(log_file_name)

// TODO:

// The unique API for recording logs
#define LOG(...) logger::Logger::GetLogger()->RecordLogs(__VA_ARGS__)

namespace logger {

// relevant to Log_level_info_prefix
enum LogLevel {
  kDebug = 0,
  kWarn,
  kError,
};
namespace {
// relevant to LogLevel
const std::string Log_level_info_prefix[3]{
    "Log(Debug): ", "Log(Warn): ", "Log(Error): "};
}  // namespace

/**
 * @brief // TODO:
 *
 */
class Logger : NonCopyableMovable {
 public:
  typedef std::shared_ptr<Logger> LoggerPtr;
  typedef std::thread ThreadPtr;
  typedef std::array<std::string, configurations::kLogBufferSize> LogBuffer;

  void EndLogger();
  static LoggerPtr GetLogger();

  void RecordLogs(LogLevel log_type, const char* log_info);
  void RecordLogs(LogLevel log_type, const std::string& log_info);
  void RecordLogs(LogLevel log_type, std::string&& log_info);

 protected:
  Logger(/* TODO: */);
  ~Logger() {}

 private:
  static void DestroyLogger(Logger* logger);

  void StartLogger(const std::string& log_file_name);
  void StartLogger(std::string&& log_file_name);

  void UpdateLoggerTime();

  // Called to write down logs
  void WriteDownLogs();

  void RecordLogs(const char* log_info);
  void RecordLogs(const std::string& log_info);

  // Actual log-record method (uses "move" semantics)
  void RecordLogs(std::string&& log_info);

  static LoggerPtr logger_;

  std::mutex log_mutex_;
  std::condition_variable log_cond_var_;
  int64_t cur_log_file_byte_;
  int64_t cur_log_file_seq_;
  std::string log_file_name_;
  ::FILE* log_file_;
  // TODO:
  // A lock-free ring buffer of log-msg ("Disruptor")
  LogBuffer log_buffer_;
};
}  // namespace logger
}  // namespace taotu

#endif  // !TAOTU_TAOTU_LOGGING_H_
