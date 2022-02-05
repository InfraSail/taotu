/**
 * @file server.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_SERVER_H_
#define TAOTU_TAOTU_SERVER_H_

#include <memory>

#include "net_address.h"
#include "non_copyable_movable.h"

namespace taotu {

class ReactorManager;

/**
 * @brief  // TODO:
 *
 */
class Server : NonCopyableMovable {
 public:
  Server(const NetAddress& listen_address, int io_thread_amount = 6,
         bool should_reuse_port = false);
  ~Server();

 private:
  typedef std::unique_ptr<ReactorManager> ReactorManagerPtr;

  ReactorManagerPtr reactor_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_SERVER_H_
