/**
 * @file memory_pool.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-02-15
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_SRC_MEMORY_POOL_H_
#define TAOTU_SRC_MEMORY_POOL_H_

#include <stddef.h>

#include "non_copyable_movable.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
struct MemeryBlock {
  union {
    char data_;
    MemeryBlock* next_node_;
  };
};

/**
 * @brief  // TODO:
 *
 * @tparam Size
 */
template <size_t Size>
class MemoryPool : NonCopyableMovable {
 public:
  MemoryPool();
  ~MemoryPool();

 private:
};

}  // namespace taotu

#endif  // !TAOTU_SRC_MEMORY_POOL_H_
