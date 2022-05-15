/**
 * @file client_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the chat client (the chat room means that when a
 * client sends a message to the server this message will be sent to all other
 * clients).
 * @date 2022-03-23
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <iostream>
#include <string>

#include "chat_client.h"

// Call it by:
// './chat_client [IP] port'
// It get messages which users type into the screen by the keyboard.
int main(int argc, char* argv[]) {
  taotu::START_LOG("chat_client_log.txt");
  if (1 == argc) {
    ::fprintf(stderr, "Usage: client [host_ip] <port>\n");
    return 0;
  } else if (2 == argc) {
    ChatClient chat_client{taotu::NetAddress{std::string{argv[1]}, 4567}};
    chat_client.Connect();
    std::string line;
    while (std::getline(std::cin, line)) {
      chat_client.Write(line);
    }
    chat_client.Disconnect();
  } else {
    ChatClient chat_client{taotu::NetAddress{
        std::string{argv[1]},
        static_cast<uint16_t>(std::stoi(std::string{argv[2]}))}};
    chat_client.Connect();
    std::string line;
    while (std::getline(std::cin, line)) {
      chat_client.Write(line);
    }
    chat_client.Disconnect();
  }
  return 0;
}
