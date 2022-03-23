/**
 * @file server_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-23
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "chat_server.h"

int main(int argc, char* argv[]) {
  if (1 == argc) {
    ChatServer chat_server{taotu::NetAddress{4567}, false};
    chat_server.Start();
  } else if (2 == argc) {
    ChatServer chat_server{taotu::NetAddress{static_cast<uint16_t>(
                               std::stoi(std::string{argv[1]}))},
                           false};
    chat_server.Start();
  } else if (3 == argc) {
    ChatServer chat_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    chat_server.Start();
  } else {
    ChatServer chat_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]})),
        static_cast<size_t>(std::stoi(std::string{argv[3]}))};
    chat_server.Start();
  }
  return 0;
}
