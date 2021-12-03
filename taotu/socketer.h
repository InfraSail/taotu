/**
 * @file socketer.h
 * @author Sigma711 (sigma711@foxmail.com)
 * @brief  // TODO:
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_SOCKETER_H_
#define TAOTU_TAOTU_SOCKETER_H_

#include <functional>
#include <memory>

#include "eventer.h"
#include "non_copyable_movable.h"

namespace taotu {

class Eventer;

/**
 * @brief  // TODO:
 *
 */
class Socketer : NonCopyableMovable {
 public:
  typedef std::function<void()> NomalCallBack;
  typedef std::function<void()> ReadCallBack;
  typedef std::weak_ptr<Eventer> EventerPtr;

  Socketer(std::shared_ptr<Eventer> eventer, int fd);
  ~Socketer();

 private:
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_SOCKETER_H_
