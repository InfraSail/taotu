/**
 * @file non_copyable_movable.h
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief
 * @date 2021-11-25
 *
 * @license: MIT
 * @copyright Copyright (c) 2021 Sigma711
 *
 */
#ifndef TAOTU_TAOTU_NON_COPYABLE_MOVABLE_H_
#define TAOTU_TAOTU_NON_COPYABLE_MOVABLE_H_

namespace taotu {

/**
 * @brief
 *
 */
class NonCopyableMovable {
 protected:
  NonCopyableMovable() = default;
  ~NonCopyableMovable() = default;

 private:
  NonCopyableMovable(const NonCopyableMovable&) = delete;
  NonCopyableMovable& operator=(const NonCopyableMovable&) = delete;
  NonCopyableMovable(NonCopyableMovable&&) = delete;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_NON_COPYABLE_MOVABLE_H_
