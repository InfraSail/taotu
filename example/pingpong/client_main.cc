/**
 * @file client_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <stdio.h>

#include <string>

#include "pingpong_client.h"

int main(int argc, char* argv[]) {
  if (argc != 7) {
    ::fprintf(stderr,
              "Usage: client <host_ip> <port> <threads> <blocksize> <sessions> "
              "<time>\n");
  } else {
    PingpongClient pingpong_client{
        taotu::NetAddress{std::string{argv[1]},
                          static_cast<uint16_t>(::atoi(argv[2]))},
        static_cast<size_t>(::atoi(argv[4])),
        static_cast<size_t>(::atoi(argv[5])), ::atoi(argv[6]),
        static_cast<size_t>(::atoi(argv[3]))};
    pingpong_client.Start();
  }
  return 0;
}
