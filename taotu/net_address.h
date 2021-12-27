/**
 * @file net_address.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_NET_ADDRESS_H_
#define TAOTU_TAOTU_NET_ADDRESS_H_

#include <netinet/in.h>
#include <stdint.h>

#include <string>

namespace taotu {

class NetAddress {
 public:
  explicit NetAddress(uint16_t port = 0, bool loop_back = false,
                      bool use_ipv6 = false);
  NetAddress(std::string ip, uint16_t port, bool use_ipv6 = false);
  explicit NetAddress(const struct sockaddr_in& socket_address)
      : socket_address_(socket_address) {}
  explicit NetAddress(const struct sockaddr_in6& socket_address6)
      : socket_address6_(socket_address6) {}

  sa_family_t GetFamily() const { return socket_address_.sin_family; }
  std::string GetIp() const;
  uint16_t GetPort() const;

  const struct sockaddr* GetNetAddress() const {
    return reinterpret_cast<const struct sockaddr*>(&socket_address6_);
  };
  void SetNetAddress6(const struct sockaddr_in6& socket_address6) {
    socket_address6_ = socket_address6;
  }

 private:
  union {
    struct sockaddr_in socket_address_;
    struct sockaddr_in6 socket_address6_;
  };
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_NET_ADDRESS_H_
