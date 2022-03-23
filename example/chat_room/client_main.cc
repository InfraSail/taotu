/**
 * @file client_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-23
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <iostream>
#include <string>

#include "chat_client.h"

int main(int argc, char* argv[]) {
  if (1 == argc) {
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
