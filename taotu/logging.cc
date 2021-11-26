/**
 * @file logging.cc
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief // TODO:
 * @date 2021-11-23
 *
 * @license: MIT
 * @copyright Copyright (c) 2021 Sigma711
 *
 */
#include "logging.h"

#include <utility>

namespace taotu {

logger::Logger::LoggerPtr logger::Logger::logger_(
    new logger::Logger, logger::Logger::DestroyLogger);
bool logger::Logger::is_initialized_ = false;

namespace logger {

Logger::LoggerPtr Logger::GetLogger() { return logger_; }
void Logger::DestroyLogger(Logger* logger) { delete logger; }

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

void Logger::StartLogger(const std::string& log_file_name) {
  StartLogger(std::move(const_cast<std::string&>(log_file_name)));
}
void Logger::StartLogger(std::string&& log_file_name) {
  if (!is_initialized_) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (!is_initialized_) {
      log_file_name_ = log_file_name;
      if (log_file_name_.empty() ||
          ((log_file_ = ::fopen(log_file_name_.c_str(), "wb")) == NULL)) {
        log_file_name_ = configurations::kLogName;
        log_file_ = ::fopen(std::string{std::string('n', 1) +
                                        std::to_string(cur_log_file_seq_ & 1) +
                                        std::string('_', 1) + log_file_name_}
                                .c_str(),
                            "wb");
      }
      std::string file_header{"Cur file sequence: 0\n"};
      ::fwrite(file_header.c_str(), file_header.size(), 1, log_file_);
      ::fflush(log_file_);
      is_initialized_ = true;
      thread_.reset(new std::thread{&Logger::WriteDownLogs, this});
    }
  }
  time_t tmp_time = 0;
}

void Logger::UpdateLoggerTime() {
  // TODO:
}

void Logger::WriteDownLogs() {
  // TODO:
}

void Logger::RecordLogs(const char* log_info) {
  RecordLogs(std::move(std::string(log_info)));
}
void Logger::RecordLogs(const std::string& log_info) {
  RecordLogs(std::move(const_cast<std::string&>(log_info)));
}

void Logger::RecordLogs(std::string&& log_info) {}

Logger::Logger(/* TODO: */) {
  // TODO:
}

}  // namespace logger
}  // namespace taotu
