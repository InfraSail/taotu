/**
 * @file connection.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_CONNECTION_H_
#define TAOTU_TAOTU_CONNECTION_H_

#include <memory>

#include "eventer.h"
#include "non_copyable_movable.h"

namespace taotu {

class Eventer;

/**
 * @brief  // TODO:
 *
 */
class Connection : NonCopyableMovable,
                   public std::__enable_shared_from_this<Connection> {
 public:
  Connection();
  ~Connection();

 private:
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTION_H_
