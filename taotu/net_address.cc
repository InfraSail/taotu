/**
 * @file socket_address.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "socket_address.h"

#include <arpa/inet.h>
#include <string.h>

#include "logger.h"

using namespace taotu;

SocketAddress::SocketAddress(uint16_t port = 0, bool loop_back = false,
                             bool use_ipv6 = false) {
  if (use_ipv6) {
    ::memset(&socket_address6_, 0, sizeof(socket_address6_));
    socket_address6_.sin6_family = AF_INET6;
    socket_address6_.sin6_addr = loop_back ? in6addr_loopback : in6addr_any;
    socket_address6_.sin6_port = ::htons(port);
  } else {
    ::memset(&socket_address_, 0, sizeof(socket_address_));
    socket_address_.sin_family = AF_INET;
    socket_address_.sin_addr.s_addr =
        ::htonl(loop_back ? INADDR_LOOPBACK : INADDR_ANY);
    socket_address_.sin_port = ::htons(port);
  }
}
SocketAddress::SocketAddress(std::string ip, uint16_t port,
                             bool use_ipv6 = false) {
  if (use_ipv6 || ::strchr(ip.c_str(), ':')) {
    ::memset(&socket_address6_, 0, sizeof(socket_address6_));
    socket_address6_.sin6_family = AF_INET6;
    if (::inet_pton(AF_INET6, ip.c_str(), &socket_address6_.sin6_addr) <= 0) {
      LOG(logger::kError, "IPv6 presentation format (" + ip +
                              ") failed to convert to network format!!!");
    }
    socket_address6_.sin6_port = ::htons(port);
  } else {
    ::memset(&socket_address_, 0, sizeof(socket_address_));
    socket_address_.sin_family = AF_INET;
    if (::inet_pton(AF_INET, ip.c_str(), &socket_address_.sin_addr) <= 0) {
      LOG(logger::kError, "IPv4 presentation format (" + ip +
                              ") failed to convert to network format!!!");
    }
    socket_address_.sin_port = ::htons(port);
  }
}

std::string SocketAddress::GetIp() const {
  char ip[64]{""};
  if (socket_address_.sin_family == AF_INET6) {
    ::inet_ntop(AF_INET6, &socket_address6_, ip, sizeof(ip));
  } else if (socket_address_.sin_family == AF_INET) {
    ::inet_ntop(AF_INET, &socket_address_, ip, sizeof(ip));
  }
  return std::string{ip};
}
uint16_t SocketAddress::GetPort() const {
  return ::htons(socket_address_.sin_port);
}
