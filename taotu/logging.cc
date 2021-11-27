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

void Logger::EndLogger() {
  logger_->is_stopping_ = 1L;
  log_cond_var_.notify_one();
  ::fclose(log_file_);
  is_initialized_ = false;
}

void Logger::StartLogger(const std::string& log_file_name) {
  StartLogger(std::move(const_cast<std::string&>(log_file_name)));
}
void Logger::StartLogger(std::string&& log_file_name) {
  if (!is_initialized_) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (!is_initialized_) {
      log_file_name_ = log_file_name;
      // Use the name of the log tile given by the project instead of the
      // unavailable one given by user
      if (log_file_name_.empty() ||
          ((log_file_ = ::fopen(log_file_name_.c_str(), "wb")) == NULL)) {
        log_file_name_ = configurations::kLogName;
        log_file_ =
            ::fopen(std::string{"n" + std::to_string(cur_log_file_seq_ & 1) +
                                "_" + log_file_name_}
                        .c_str(),
                    "wb");
      }
      std::string file_header{"Current file sequence: 0\n"};
      ::fwrite(file_header.c_str(), file_header.size(), 1, log_file_);
      ::fflush(log_file_);
      is_initialized_ = true;
      thread_.reset(new std::thread{&Logger::WriteDownLogs, this},
                    [](std::thread* trd) {
                      if (trd != nullptr && trd->joinable()) {
                        trd->join();
                        delete trd;
                        trd = nullptr;
                      }
                    });
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
  time_t tmp_time;
  ::time(&tmp_time);
  if (tmp_time > time_now_sec_) {
    std::lock_guard<std::mutex> lock(time_mutex_);
    ::time(&tmp_time);
    if (tmp_time > time_now_sec_) {
      time_now_sec_ = tmp_time;
      // In consideration of time zone
      struct tm* tmp_tm = ::localtime(&tmp_time);
      std::string tmp_time_now_str(::asctime(tmp_tm));
      tmp_time_now_str.resize(tmp_time_now_str.size() - 1);
      time_now_str_ = std::move("[ " + tmp_time_now_str + " ]");
    }
  }
}

void Logger::WriteDownLogs() {
  // Loop for flushing io buffer into disk
  while (true) {
    if (is_stopping_ == 1L && read_index_ == wrote_index_) {
      return;
    } else {
      // Loop for flushing ring buffer into io buffer
      while (read_index_ < wrote_index_) {
        int cur_read_index = read_index_ + 1;
        // Change the log file to new one when the old is full (Always only 2
        // log files in circulation)
        if (cur_log_file_byte_ >= kStandardLogFileByte) {
          ::fflush(log_file_);
          ::fclose(log_file_);
          ++cur_log_file_seq_;
          log_file_ =
              ::fopen(std::string{"n" + std::to_string(cur_log_file_seq_ & 1) +
                                  "_" + log_file_name_}
                          .c_str(),
                      "wb");
          cur_log_file_byte_ = 0;
          std::string file_header{"Current file sequence: " +
                                  std::to_string(cur_log_file_seq_) + "\n"};
          ::fwrite(file_header.c_str(), file_header.size(), 1, log_file_);
        }
        // Copy the content of one bucket of ring buffer to io buffer
        std::string& tmp_buf =
            log_buffer_[read_index_ & (configurations::kLogBufferSize - 1)];
        int tmp_buf_len = tmp_buf.size();
        ::fwrite(tmp_buf.c_str(), tmp_buf_len, 1, log_file_);
        cur_log_file_byte_ += tmp_buf_len;
        tmp_buf.clear();
        read_index_ = cur_read_index;
      }
      // Flush into disk (the status of ring buffer may change because of
      // spending much time here)
      ::fflush(log_file_);
      // Sleep when the buffer is empty
      if (read_index_ == wrote_index_) {
        std::unique_lock<std::mutex> lock(log_mutex_);
        if (is_stopping_ == 0L && read_index_ == wrote_index_) {
          log_cond_var_.wait(lock);
        }
      }
    }
  }
}

void Logger::RecordLogs(const char* log_info) {
  RecordLogs(std::move(std::string(log_info)));
}
void Logger::RecordLogs(const std::string& log_info) {
  RecordLogs(std::move(const_cast<std::string&>(log_info)));
}

void Logger::RecordLogs(std::string&& log_info) {
  // Give up recording this time because the logs which have been in the file
  // are more valuable
  if (write_index_.load(std::memory_order_acquire) - read_index_ >=
      configurations::kLogBufferSize - 1) {
    return;
  }
  const int64_t write_index =
      write_index_.fetch_add(1, std::memory_order_release);  // Old value
  UpdateLoggerTime();
  // TODO:
}

Logger::Logger()
    : cur_log_file_byte_(0),
      cur_log_file_seq_(0),
      log_file_(NULL),
      thread_(nullptr),
      is_stopping_(0L),
      read_index_(-1L),
      wrote_index_(-1L),
      write_index_(0L),
      time_now_sec_(0) {}

}  // namespace logger
}  // namespace taotu
