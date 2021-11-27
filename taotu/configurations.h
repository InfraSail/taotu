/**
 * @file configurer.h
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief // TODO:
 * @date 2021-11-25
 *
 * @license: MIT
 * @copyright Copyright (c) 2021 Sigma711
 *
 */
#ifndef TAOTU_TAOTU_CONFIGURATIONS_H_
#define TAOTU_TAOTU_CONFIGURATIONS_H_

#include <string>

namespace taotu {
namespace configurations {
static const std::string kLogName{"log.txt"};

// 2GB
static constexpr int64_t kLogFileMaxByte = 1024 * 1024 * 1024;

// Should be the nth power of 2 (for "Disruptor")
static constexpr int64_t kLogBufferSize = 1024 * 1024 * 16;
// TODO:
}  // namespace configurations

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONFIGURATIONS_H_
