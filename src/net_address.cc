/**
 * @file net_address.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "NetAddress" which is the encapsulation of net
 * address info.
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "net_address.h"

#include <arpa/inet.h>
#include <string.h>

#include "logger.h"

namespace taotu {

NetAddress::NetAddress(uint16_t port, bool loop_back, bool use_ipv6) {
  if (use_ipv6) {
    ::memset(&socket_address6_, 0, sizeof(socket_address6_));
    socket_address6_.sin6_family = AF_INET6;
    socket_address6_.sin6_addr = loop_back ? in6addr_loopback : in6addr_any;
    socket_address6_.sin6_port = htons(port);
  } else {
    ::memset(&socket_address_, 0, sizeof(socket_address_));
    socket_address_.sin_family = AF_INET;
    socket_address_.sin_addr.s_addr =
        htonl(loop_back ? INADDR_LOOPBACK : INADDR_ANY);
    socket_address_.sin_port = htons(port);
  }
}
NetAddress::NetAddress(const std::string& ip, uint16_t port, bool use_ipv6) {
  if (use_ipv6 || ::strchr(ip.c_str(), ':')) {
    ::memset(&socket_address6_, 0, sizeof(socket_address6_));
    socket_address6_.sin6_family = AF_INET6;
    if (::inet_pton(AF_INET6, ip.c_str(), &socket_address6_.sin6_addr) <= 0) {
      LOG_ERROR(
          "IPv6 presentation format (%s) failed to convert to network "
          "format!!!",
          ip.c_str());
    }
    socket_address6_.sin6_port = htons(port);
  } else {
    ::memset(&socket_address_, 0, sizeof(socket_address_));
    socket_address_.sin_family = AF_INET;
    if (::inet_pton(AF_INET, ip.c_str(), &socket_address_.sin_addr) <= 0) {
      LOG_ERROR(
          "IPv4 presentation format (%s) failed to convert to network "
          "format!!!",
          ip.c_str());
    }
    socket_address_.sin_port = htons(port);
  }
}

std::string NetAddress::GetIp() const {
  char ip[64]{""};
  if (GetFamily() == AF_INET6) {
    ::inet_ntop(AF_INET6, &socket_address6_, ip, sizeof(ip));
  } else if (GetFamily() == AF_INET) {
    ::inet_ntop(AF_INET, &socket_address_, ip, sizeof(ip));
  }
  return std::string{ip};
}
uint16_t NetAddress::GetPort() const { return htons(socket_address_.sin_port); }

}  // namespace taotu
