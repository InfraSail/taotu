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

#include <functional>
#include <utility>

namespace taotu {
namespace logger {

std::atomic<bool> Logger::is_initialized{false};

Logger* Logger::GetLogger(bool should_start) {
  // The unique actual "Logger" object
  static Logger logger;
  if (should_start) {
    logger.StartLogger(kLogName);
  }
  return &logger;
}

void Logger::EndLogger() {
  is_stopping_.store(true, std::memory_order_release);
  log_cond_var_.notify_one();
  if (thread_.joinable()) {
    thread_.join();
  }
  ::fclose(log_file_);
  is_initialized.store(false, std::memory_order_release);
}

void Logger::StartLogger(const std::string& log_file_name) {
  StartLogger(std::move(const_cast<std::string&>(log_file_name)));
}
void Logger::StartLogger(std::string&& log_file_name) {
  if (!is_initialized.load(std::memory_order_acquire)) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (!is_initialized.load(std::memory_order_acquire)) {
      is_stopping_.store(false, std::memory_order_release);
      write_index_.store(0, std::memory_order_relaxed);
      read_index_.store(0, std::memory_order_relaxed);
      pending_.store(0, std::memory_order_relaxed);
      for (size_t i = 0; i < kLogBufferSize; ++i) {
        log_buffer_[i].seq.store(i, std::memory_order_relaxed);
        log_buffer_[i].data.clear();
      }
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
      is_initialized.store(true, std::memory_order_release);
      thread_ = std::thread([this]() { this->WriteDownLogs(); });
    }
  }
  UpdateLoggerTime();
}

void Logger::RecordLogs(LogLevel log_type, const std::string& log_info) {
  RecordLogs(Log_level_info_prefix[log_type] + log_info);
}
void Logger::RecordLogs(LogLevel log_type, std::string&& log_info) {
  RecordLogs(Log_level_info_prefix[log_type] + log_info);
}

std::string Logger::UpdateLoggerTime() {
  time_t tmp_time;
  ::time(&tmp_time);
  std::lock_guard<std::mutex> lock(time_mutex_);
  if (tmp_time > time_now_sec_) {
    time_now_sec_ = tmp_time;
    // In consideration of time zone
    struct tm* tmp_tm = ::localtime(&tmp_time);
    std::string tmp_time_now_str(::asctime(tmp_tm));
    tmp_time_now_str.resize(tmp_time_now_str.size() - 1);
    time_now_str_ = "[ " + tmp_time_now_str + " ]";
  }
  return time_now_str_;
}

void Logger::WriteDownLogs() {
  // Loop for flushing io buffer into disk
  while (!is_stopping_.load(std::memory_order_acquire) ||
         pending_.load(std::memory_order_acquire) != 0) {
    // Drain available logs.
    std::string tmp_buf;
    while (Dequeue(&tmp_buf)) {
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
      size_t tmp_buf_len = tmp_buf.size();
      ::fwrite(tmp_buf.c_str(), tmp_buf_len, 1, log_file_);
      cur_log_file_byte_ += static_cast<int64_t>(tmp_buf_len);
      tmp_buf.clear();
    }
    // Flush into disk (the status of ring buffer may change because of
    // spending much time here)
    ::fflush(log_file_);
    // Block when the buffer is empty
    if (pending_.load(std::memory_order_acquire) == 0) {
      std::unique_lock<std::mutex> lock(log_mutex_);
      if (!is_stopping_.load(std::memory_order_acquire) &&
          pending_.load(std::memory_order_acquire) == 0) {
        log_cond_var_.wait(lock);
      }
    }
  }
}

void Logger::RecordLogs(const char* log_info) {
  RecordLogs(std::string(log_info));
}
void Logger::RecordLogs(const std::string& log_info) {
  RecordLogs(std::move(const_cast<std::string&>(log_info)));
}

void Logger::RecordLogs(std::string&& log_info) {
  // Splice this log record
  std::string time_now_str{UpdateLoggerTime()};
  std::string log_data(time_now_str.size() + log_info.size() + 2, ' ');
  ::memcpy(reinterpret_cast<void*>(const_cast<char*>(log_data.c_str())),
           time_now_str.c_str(), time_now_str.size());
  ::memcpy(reinterpret_cast<void*>(const_cast<char*>(log_data.c_str()) +
                                   time_now_str.size() + 1),
           log_info.c_str(), log_info.size());
  log_data.back() = '\n';
  // Put this log record into ring buffer (drop if full)
  (void)Enqueue(std::move(log_data));
}

bool Logger::Enqueue(std::string&& log_data) {
  size_t pos = write_index_.load(std::memory_order_relaxed);
  for (;;) {
    LogSlot& slot = log_buffer_[pos & kLogBufferMask];
    size_t seq = slot.seq.load(std::memory_order_acquire);
    intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
    if (diff == 0) {
      if (write_index_.compare_exchange_weak(pos, pos + 1,
                                             std::memory_order_relaxed)) {
        slot.data = std::move(log_data);
        slot.seq.store(pos + 1, std::memory_order_release);
        size_t prev = pending_.fetch_add(1, std::memory_order_release);
        if (prev == 0) {
          std::lock_guard<std::mutex> lock(log_mutex_);
          log_cond_var_.notify_one();
        }
        return true;
      }
    } else if (diff < 0) {
      return false;  // queue full, drop log
    } else {
      pos = write_index_.load(std::memory_order_relaxed);
    }
  }
}

bool Logger::Dequeue(std::string* out) {
  size_t pos = read_index_.load(std::memory_order_relaxed);
  for (;;) {
    LogSlot& slot = log_buffer_[pos & kLogBufferMask];
    size_t seq = slot.seq.load(std::memory_order_acquire);
    intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
    if (diff == 0) {
      if (read_index_.compare_exchange_weak(pos, pos + 1,
                                            std::memory_order_relaxed)) {
        out->swap(slot.data);
        slot.data.clear();
        slot.seq.store(pos + kLogBufferSize, std::memory_order_release);
        pending_.fetch_sub(1, std::memory_order_release);
        return true;
      }
    } else if (diff < 0) {
      return false;  // queue empty
    } else {
      pos = read_index_.load(std::memory_order_relaxed);
    }
  }
}

Logger::Logger()
    : is_stopping_(false),
      cur_log_file_byte_(0),
      cur_log_file_seq_(0),
      log_file_(NULL),
      time_now_sec_(0),
      write_index_(0),
      read_index_(0),
      pending_(0) {
  (void)filler1_;
  (void)filler2_;
  (void)filler3_;
  (void)filler4_;
  for (size_t i = 0; i < kLogBufferSize; ++i) {
    log_buffer_[i].seq.store(i, std::memory_order_relaxed);
  }
}

Logger::~Logger() {
  if (thread_.joinable()) {
    thread_.join();
  }
  is_initialized.store(false, std::memory_order_release);
}

}  // namespace logger
}  // namespace taotu
