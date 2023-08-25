/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the simple discard server (always discards the
 * messages clients sent).
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "discard.h"

// Call it by:
// './simple_discard [port [amount-of-I/O-threads]]'
int main(int argc, char* argv[]) {
  taotu::START_LOG("simple_discard_log.txt");
  if (1 == argc) {
    DiscardServer discard_server{taotu::NetAddress{4567}, false};
    discard_server.Start();
  } else if (2 == argc) {
    DiscardServer discard_server{taotu::NetAddress{static_cast<uint16_t>(
                                     std::stoi(std::string{argv[1]}))},
                                 false};
    discard_server.Start();
  } else {
    DiscardServer discard_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    discard_server.Start();
  }
  return 0;
}
