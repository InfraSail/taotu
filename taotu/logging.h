/**
 * @file logging.h
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief
 * @date 2021-11-23
 *
 * @license: MIT
 * @copyright Copyright (c) 2021 Sigma711
 *
 */
#ifndef TAOTU_TAOTU_LOGGING_H_
#define TAOTU_TAOTU_LOGGING_H_

#include <memory>
#include <mutex>
#include <string>

#include "non_copyable_movable.h"

namespace taotu {
namespace logger {

// relevant to Log_level_info_prefix
enum LogLevel {
  kDebug = 0,
  kWarn,
  kError,
};
namespace {
// relevant to LogLevel
std::string Log_level_info_prefix[3]{
    "Log (Debug) : ", "Log (Warn) : ", "Log (Error) : "};
}  // namespace

/**
 * @brief
 *
 */
class Logger : utility::NonCopyableMovable {
 public:
  typedef std::unique_ptr<Logger> LoggerPtr;

  bool StartLogger(const std::string&& log_file);
  void EndLogger();
  static LoggerPtr GetLogger();

  void RecordLogs(LogLevel log_type, const char* log_info);
  void RecordLogs(LogLevel log_type, const std::string& log_info);
  void RecordLogs(LogLevel log_type, std::string&& log_info);

 protected:
  Logger();
  ~Logger();

 private:
  void UpdateLoggerTime();
  void RecordLogs();  // Finally call it when write down logs

  void RecordLogs(const char* log_info);
  void RecordLogs(const std::string& log_info);
  void RecordLogs(std::string&& log_info);

  static LoggerPtr logger_;
  static std::mutex log_mutex_;
  static bool is_initialized;
};
}  // namespace logger
}  // namespace taotu

#endif  // !TAOTU_TAOTU_LOGGING_H_
