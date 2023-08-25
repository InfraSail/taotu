/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the simple time server (always sends the current
 * time).
 * @date 2022-03-03
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "time.h"

// Call it by:
// './simple_time [port [amount-of-I/O-threads]]'
int main(int argc, char* argv[]) {
  taotu::START_LOG("simple_time_log.txt");
  if (1 == argc) {
    TimeServer time_server{taotu::NetAddress{4567}, false};
    time_server.Start();
  } else if (2 == argc) {
    TimeServer time_server{taotu::NetAddress{static_cast<uint16_t>(
                               std::stoi(std::string{argv[1]}))},
                           false};
    time_server.Start();
  } else {
    TimeServer time_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    time_server.Start();
  }
  return 0;
}
