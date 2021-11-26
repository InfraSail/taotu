/**
 * @file logging.cc
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief
 * @date 2021-11-23
 *
 * @license: MIT
 * @copyright Copyright (c) 2021 Sigma711
 *
 */
#include "logging.h"

#include <utility>

#include "configurations.h"

namespace taotu {

logger::Logger::LoggerPtr logger::Logger::logger_ = nullptr;
std::mutex logger::Logger::log_mutex_;

namespace logger {

void Logger::EndLogger() {}

Logger::LoggerPtr Logger::GetLogger(std::string&& log_file) {
  if (!logger_) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (!logger_) {
      logger_.reset(new Logger);
      logger_->StartLogger(log_file);
    }
  }
  return logger_;
}

void Logger::RecordLogs(LogLevel log_type, const char* log_info) {
  RecordLogs(
      std::move(Log_level_info_prefix[log_type] + std::string(log_info)));
}
void Logger::RecordLogs(LogLevel log_type, const std::string& log_info) {
  RecordLogs(std::move(Log_level_info_prefix[log_type] + log_info));
}
void Logger::RecordLogs(LogLevel log_type, std::string&& log_info) {
  RecordLogs(std::move(Log_level_info_prefix[log_type] + log_info));
}

bool Logger::StartLogger(std::string& log_file) {
  if (log_file.empty()) {
    log_file = configurations::kLogName;
  }
}
void Logger::WriteDownLogs() {}
void Logger::UpdateLoggerTime() {}

void Logger::RecordLogs(const char* log_info) {
  RecordLogs(std::move(std::string(log_info)));
}
void Logger::RecordLogs(const std::string& log_info) {
  RecordLogs(std::move(std::string(log_info)));
}
void Logger::RecordLogs(std::string&& log_info) {}

Logger::Logger() {}
Logger::~Logger() {}

}  // namespace logger
}  // namespace taotu
