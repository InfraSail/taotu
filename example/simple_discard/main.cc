/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "discard.h"

int main(int argc, char* argv[]) {
  if (1 == argc) {
    DiscardServer discard_server{taotu::NetAddress{4567}, false};
    discard_server.Start();
  } else if (2 == argc) {
    DiscardServer discard_server{taotu::NetAddress{static_cast<uint16_t>(
                                     std::stoi(std::string{argv[1]}))},
                                 false};
    discard_server.Start();
  } else if (3 == argc) {
    DiscardServer discard_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    discard_server.Start();
  } else {
    DiscardServer discard_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]})),
        static_cast<size_t>(std::stoi(std::string{argv[3]}))};
    discard_server.Start();
  }
  return 0;
}
