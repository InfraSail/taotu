/**
 * @file logger.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Logger" which is the log.
 * recorder of this library.
 * @date 2021-11-23
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "logger.h"

#include <utility>

using namespace taotu;
using namespace taotu::logger;

// The unique actual "Logger" object
Logger::LoggerPtr Logger::logger_ptr(new Logger, Logger::DestroyLogger);
bool Logger::is_initialized = false;

Logger::LoggerPtr Logger::GetLogger() { return logger_ptr; }
void Logger::DestroyLogger(Logger* logger_raw_ptr) { delete logger_raw_ptr; }

void Logger::EndLogger() {
  is_stopping_ = 1L;
  log_cond_var_.notify_one();
  if (thread_->joinable()) {
    thread_->join();
  }
  ::fclose(log_file_);
  is_initialized = false;
}

void Logger::StartLogger(const std::string& log_file_name) {
  StartLogger(std::move(const_cast<std::string&>(log_file_name)));
}
void Logger::StartLogger(std::string&& log_file_name) {
  if (!is_initialized) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (!is_initialized) {
      log_file_name_ = log_file_name;
      // Use the name of the log tile given by the project instead of the
      // unavailable one given by user
      if (log_file_name_.empty() ||
          ((log_file_ = ::fopen(log_file_name_.c_str(), "wb")) == NULL)) {
        log_file_name_ = kLogName;
        log_file_ =
            ::fopen(std::string{"n" + std::to_string(cur_log_file_seq_ & 1) +
                                "_" + log_file_name_}
                        .c_str(),
                    "wb");
      }
      std::string file_header{"Current file sequence: 0\n"};
      ::fwrite(file_header.c_str(), file_header.size(), 1, log_file_);
      ::fflush(log_file_);
      is_initialized = true;
      thread_ = std::make_unique<std::thread>(
          new std::thread{&Logger::WriteDownLogs, this}, [](std::thread* trd) {
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
      // Loop for flushing ring buffer into io buffer (flushing operation is
      // very time-consuming so that something may happen in another thread
      // during this time)
      while (read_index_ < wrote_index_) {
        int64_t cur_read_index = read_index_ + 1;
        // Change the log file to new one when the old is full (Always only
        // 2 log files in circulation)
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
            log_buffer_[cur_read_index & (kLogBufferSize - 1)];
        int tmp_buf_len = tmp_buf.size();
        ::fwrite(tmp_buf.c_str(), tmp_buf_len, 1, log_file_);
        cur_log_file_byte_ += tmp_buf_len;
        tmp_buf.clear();
        // Update the index which was read last time
        read_index_ = cur_read_index;
      }
      // Flush into disk (the status of ring buffer may change because of
      // spending much time here)
      ::fflush(log_file_);
      // Block when the buffer is empty
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
  if (write_index_.load() - read_index_ >= kLogBufferSize - 1) {
    return;
  }
  // Update the index which can be written next time and get the previous value
  const int64_t write_index = write_index_.fetch_add(1);
  UpdateLoggerTime();
  // Splice this log record
  std::string time_now_str{time_now_str_.c_str()};
  std::string log_data(time_now_str.size() + log_info.size() + 2, ' ');
  ::memcpy(const_cast<char*>(log_data.c_str()), time_now_str.c_str(),
           time_now_str.size());
  ::memcpy(const_cast<char*>(log_data.c_str()) + time_now_str.size() + 1,
           log_info.c_str(), log_info.size());
  log_data.back() = '\n';
  // Put this log record into ring buffer
  log_buffer_[write_index & (kLogBufferSize - 1)] = std::move(log_data);
  // Block for a while if ring buffer is full (a small probability event)
  while (write_index - 1L > wrote_index_) {
  }
  // Update the index which was written last time
  wrote_index_ = write_index;
  // Awake flushing thread if ring buffer was empty before
  if (write_index - 1L == read_index_) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_cond_var_.notify_one();
  }
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
