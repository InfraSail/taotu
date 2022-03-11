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

int main(int argc, char* argv[]) {
  if (1 == argc) {
    TimeServer time_server{taotu::NetAddress{4567}, false};
    time_server.Start();
  } else if (2 == argc) {
    TimeServer time_server{taotu::NetAddress{static_cast<uint16_t>(
                               std::stoi(std::string{argv[1]}))},
                           false};
    time_server.Start();
  } else if (3 == argc) {
    TimeServer time_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    time_server.Start();
  } else {
    TimeServer time_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]})),
        static_cast<size_t>(std::stoi(std::string{argv[3]}))};
    time_server.Start();
  }
  return 0;
}
