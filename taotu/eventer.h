/**
 * @file eventer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_EVENTER_H_
#define TAOTU_TAOTU_EVENTER_H_

#include <memory>

#include "non_copyable_movable.h"

namespace taotu {

class Socketer;

class Eventer : NonCopyableMovable,
                public std::__enable_shared_from_this<Eventer> {
 public:
  void RemoveSocketer(Socketer *Socketer);
  void UpdateSocketer(Socketer *Socketer);
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_EVENTER_H_
