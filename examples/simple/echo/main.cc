/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-02-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "echo.h"

int main() {
  EchoServer echo_server{taotu::NetAddress{4567, false, false}, false};
  echo_server.Start();
  return 0;
}
