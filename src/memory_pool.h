/**
 * @file memory_pool.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration and Implementation of struct "MemoryBlockNode" and class
 * "MemoryPool".
 * @date 2022-02-15
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_SRC_MEMORY_POOL_H_
#define TAOTU_SRC_MEMORY_POOL_H_

#include <stddef.h>
#include <stdlib.h>

#include "non_copyable_movable.h"

namespace taotu {

/**
 * @brief "MemoryBlockNode" is the memory block node.
 *
 */
struct MemoryBlockNode {
  union {
    char data_;
    MemoryBlockNode* next_node_;
  };
};

/**
 * @brief "MemoryPool" is the memory pool.
 *
 */
template <size_t ObjectSize>
class MemoryPool : NonCopyableMovable {
 public:
  MemoryPool()
      : free_list_head_(nullptr), list_head_(nullptr), malloc_time_(0) {
    if (ObjectSize < sizeof(MemoryBlockNode)) {
      object_size_ = sizeof(MemoryBlockNode);
    } else {
      object_size_ = ObjectSize;
    }
  }
  ~MemoryPool() {
    while (list_head_ != nullptr) {
      MemoryBlockNode* tmp_list_head = list_head_;
      list_head_ = list_head_->next_node_;
      ::free(reinterpret_cast<void*>(tmp_list_head));
    }
  }

  // Allocate a memory block in this memory pool
  void* Allocate() {
    void* return_prt = nullptr;
    if (nullptr == free_list_head_) {
      size_t malloc_size =
          MemoryPool<ObjectSize>::kInitialMallocSize + malloc_time_;
      void* new_malloc_ptr =
          ::malloc(malloc_size * object_size_ + sizeof(MemoryBlockNode));
      MemoryBlockNode* new_malloc_node =
          reinterpret_cast<MemoryBlockNode*>(new_malloc_ptr);
      new_malloc_node->next_node_ = list_head_;
      list_head_ = new_malloc_node;
      new_malloc_ptr = reinterpret_cast<void*>(
          reinterpret_cast<char*>(new_malloc_ptr) + sizeof(MemoryBlockNode));
      for (size_t i = 0; i < malloc_size; ++i) {
        MemoryBlockNode* new_node =
            reinterpret_cast<MemoryBlockNode*>(new_malloc_ptr);
        new_node->next_node_ = free_list_head_;
        free_list_head_ = new_node;
        new_malloc_ptr = reinterpret_cast<void*>(
            reinterpret_cast<char*>(new_malloc_ptr) + object_size_);
      }
      ++malloc_time_;
    }
    return_prt = reinterpret_cast<void*>(&free_list_head_->data_);
    free_list_head_ = free_list_head_->next_node_;
    return return_prt;
  }

  // Free a memory block in this memory pool
  void Deallocate(void* memory_block_ptr) {
    if (nullptr == memory_block_ptr) {
      return;
    }
    MemoryBlockNode* tmp_node =
        reinterpret_cast<MemoryBlockNode*>(memory_block_ptr);
    tmp_node->next_node_ = free_list_head_;
    free_list_head_ = tmp_node;
  }

 private:
  // Initial malloc() size each time
  static const size_t kInitialMallocSize = 40;

  // Free linked list
  MemoryBlockNode* free_list_head_;

  // Linked list of allocation
  MemoryBlockNode* list_head_;

  // Time of calling malloc()
  size_t malloc_time_;

  // Size of each memory block
  size_t object_size_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_MEMORY_POOL_H_
