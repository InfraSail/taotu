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

void Logger::EndLogger() { logger_->is_stopping_ = true; }

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
      std::string file_header{"Current file sequence: 0\n"};
      ::fwrite(file_header.c_str(), file_header.size(), 1, log_file_);
      ::fflush(log_file_);
      is_initialized_ = true;
      thread_.reset(new std::thread{&Logger::WriteDownLogs, this});
    }
  }
  UpdateLoggerTime();
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

void Logger::UpdateLoggerTime() {
  std::lock_guard lock(time_mutex_);
  time_t tmp_time;
  ::time(&tmp_time);
  if (tmp_time > time_now_sec_) {
    time_now_sec_ = tmp_time;
    struct tm* tmp_tm = ::localtime(&tmp_time);
    std::string tmp_time_now_str(::asctime(tmp_tm));
    time_now_str_ = std::move("[ " + tmp_time_now_str + " ]");
  }
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

Logger::Logger()
    : cur_log_file_byte_(0),
      cur_log_file_seq_(0),
      log_file_(NULL),
      thread_(nullptr),
      is_stopping_(0L),
      read_index_(-1L),
      wrote_index_(-1L),
      write_index_(0L),
      time_now_str_("1970-01-01 00:00:00"),
      time_now_sec_(0) {}

}  // namespace logger
}  // namespace taotu
