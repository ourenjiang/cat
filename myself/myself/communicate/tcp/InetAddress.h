#pragma once
// #include "myself/base/StringPiece.h"
#include <string>

#include <netinet/in.h>

namespace myself
{
namespace tcp
{

class InetAddress
{
 public:
  explicit InetAddress(uint16_t port);
  // InetAddress(const StringPiece& ip, uint16_t port);
  InetAddress(const std::string& ip, uint16_t port);
  InetAddress(const struct sockaddr_in& addr): addr_(addr)
  {
  }

  std::string toIp() const;
  std::string toIpPort() const;
  std::string toHostPort() const
  { return toIpPort(); }

  const struct sockaddr_in& getSockAddrInet() const { return addr_; }
  void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

  uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }
  uint16_t portNetEndian() const { return addr_.sin_port; }

 private:
  struct sockaddr_in addr_;
};

}//namespace tcp
}//namespace myself
