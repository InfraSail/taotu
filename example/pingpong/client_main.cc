/**
 * @file client_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the pingpong client (stress test).
 * @date 2022-03-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <stdio.h>

#include <memory>
#include <string>

#include "pingpong_client.h"

// Call it by:
// './pingpong_client IP port amount-of-I/O-threads size-of-block-sent
// amount-of-sessions time-for-waiting'
int main(int argc, char* argv[]) {
  taotu::START_LOG("pingpong_client_log.txt");
  if (argc != 7) {
    ::fprintf(stderr,
              "Usage: client <host_ip> <port> <threads> <blocksize> <sessions> "
              "<time>\n");
  } else {
    std::shared_ptr<PingpongClient> pingpong_client =
        std::make_shared<PingpongClient>(
            taotu::NetAddress{std::string{argv[1]},
                              static_cast<uint16_t>(::atoi(argv[2]))},
            static_cast<size_t>(::atoi(argv[4])),
            static_cast<size_t>(::atoi(argv[5])), ::atoi(argv[6]),
            static_cast<size_t>(::atoi(argv[3])));
    pingpong_client->Start();
  }
  return 0;
}
