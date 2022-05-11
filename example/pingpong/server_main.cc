/**
 * @file server_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the pingpong server (stress test).
 * @date 2022-03-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "pingpong_server.h"

// Call it by:
// './pingpong_server [port [amount-of-I/O-threads
// [amount-of-calculation-threads]]]'
int main(int argc, char* argv[]) {
  taotu::START_LOG("pingpong_server_log.txt");
  if (1 == argc) {
    PingpongServer pingpong_server{taotu::NetAddress{4567}, false};
    pingpong_server.Start();
  } else if (2 == argc) {
    PingpongServer pingpong_server{taotu::NetAddress{static_cast<uint16_t>(
                                       std::stoi(std::string{argv[1]}))},
                                   false};
    pingpong_server.Start();
  } else if (3 == argc) {
    PingpongServer pingpong_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    pingpong_server.Start();
  } else {
    PingpongServer pingpong_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]})),
        static_cast<size_t>(std::stoi(std::string{argv[3]}))};
    pingpong_server.Start();
  }
  return 0;
}
