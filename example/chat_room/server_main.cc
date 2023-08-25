/**
 * @file server_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the chat server (the chat room means that when a
 * client sends a message to the server this message will be sent to all other
 * clients).
 * @date 2022-03-23
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "chat_server.h"

// Call it by:
// './chat_server [port [amount-of-I/O-threads]]'
int main(int argc, char* argv[]) {
  taotu::START_LOG("server_main_log.txt");
  if (1 == argc) {
    ChatServer chat_server{taotu::NetAddress{4567}, false};
    chat_server.Start();
  } else if (2 == argc) {
    ChatServer chat_server{taotu::NetAddress{static_cast<uint16_t>(
                               std::stoi(std::string{argv[1]}))},
                           false};
    chat_server.Start();
  } else {
    ChatServer chat_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    chat_server.Start();
  }
  return 0;
}
