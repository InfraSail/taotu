/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the simple echo server (always returns back the
 * messages clients sent).
 * @date 2022-02-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "echo.h"

// Call it by:
// './simple_echo [port [amount-of-I/O-threads
// [amount-of-calculation-threads]]]'
int main(int argc, char* argv[]) {
  taotu::START_LOG("simple_echo_log.txt");
  if (1 == argc) {
    EchoServer echo_server{taotu::NetAddress{4567}, false};
    echo_server.Start();
  } else if (2 == argc) {
    EchoServer echo_server{taotu::NetAddress{static_cast<uint16_t>(
                               std::stoi(std::string{argv[1]}))},
                           false};
    echo_server.Start();
  } else if (3 == argc) {
    EchoServer echo_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    echo_server.Start();
  } else {
    EchoServer echo_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]})),
        static_cast<size_t>(std::stoi(std::string{argv[3]}))};
    echo_server.Start();
  }
  return 0;
}
