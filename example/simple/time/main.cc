/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-03
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "time.h"

int main() {
  TimeServer time_server{taotu::NetAddress{4567, false, false}, false};
  time_server.Start();
  return 0;
}
