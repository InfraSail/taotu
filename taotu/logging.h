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

#include <string>

#include "boost/noncopyable.hpp"

namespace taotu {

// relevant to Log_level_info_prefix
enum LogLevel {
  KDebug = 0,
  KWarn,
  KError,
};
namespace logger {
namespace {
// relevant to LogLevel
std::string Log_level_info_prefix[3]{
    "Log (Debug) : ", "Log (Warn) : ", "Log (Error) : "};
}  // namespace

/**
 * @brief
 *
 */
class Logger : boost::noncopyable {
 private:
  Logger();
  ~Logger();
};
}  // namespace logger
}  // namespace taotu

#endif
