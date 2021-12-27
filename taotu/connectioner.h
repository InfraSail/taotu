/**
 * @file connectioner.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-27
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_CONNECTIONER_H_
#define TAOTU_TAOTU_CONNECTIONER_H_

#include "eventer.h"
#include "socketer.h"

namespace taotu {

class Socketer;
class Eventer;

/**
 * @brief  // TODO:
 *
 */
class Connectioner {
 public:
 private:
  Socketer socketer_;
  Eventer eventer_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTIONER_H_
