/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "discard.h"

int main() {
  DiscardServer discard_server(taotu::NetAddress{4567, false, false}, false);
  discard_server.Start();
  return 0;
}
