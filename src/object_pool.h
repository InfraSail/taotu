/**
 * @file object_pool.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration and Implementation of class "ObjectPool".
 * @date 2022-04-19
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_SRC_OBJECT_POOL_H_
#define TAOTU_SRC_OBJECT_POOL_H_

#include <type_traits>

#include "memory_pool.h"
#include "non_copyable_movable.h"

/**
 * @brief "ObjectPool" is the object pool.
 *
 */
namespace taotu {

template <class T>
class ObjectPool : NonCopyableMovable {
 public:
  ObjectPool() {}
  ~ObjectPool() {}

  // New an object
  template <class... Args>
  T* New(Args... args) {
    return NewIf(
        std::integral_constant<bool,
                               std::is_trivially_constructible<T>::value>(),
        args...);
  }

  // Delete an object
  void Delete(void* object_ptr) {
    if (nullptr == object_ptr) {
      return;
    }
    DeleteIf(
        std::integral_constant<bool,
                               std::is_trivially_constructible<T>::value>(),
        object_ptr);
  }

 private:
  // New an object when it is trivially constructible
  template <typename... Args>
  T* NewIf(std::true_type, Args... args) {
    return reinterpret_cast<T*>(memory_pool_.Allocate());
  }

  // New an object when it is not trivially constructible
  template <typename... Args>
  T* NewIf(std::false_type, Args... args) {
    void* new_object_ptr = memory_pool_.Allocate();
    return new (new_object_ptr) T(args...);
  }

  // Delete an object when it is trivially constructible
  template <typename... Args>
  void DeleteIf(std::true_type, void* object_ptr) {
    memory_pool_.Deallocate(object_ptr);
  }

  // Delete an object when it is not trivially constructible
  template <typename... Args>
  void DeleteIf(std::false_type, void* object_ptr) {
    reinterpret_cast<T*>(object_ptr)->~T();
    memory_pool_.Deallocate(object_ptr);
  }

  // Memory pool
  MemoryPool<sizeof(T)> memory_pool_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_OBJECT_POOL_H_
