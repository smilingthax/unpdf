#ifndef NETIF_H_
#define NETIF_H_

#include "net.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace NetIf {

std::string getHostName();
Net::ip_addr getHostIp(); // i.e. ip of hostname

Net::ip_addr getDefaultSource4();
//Net::ip_addr getDefaultSource6();

struct iface_info {
  std::string name;
  std::vector<std::pair<Net::ip_addr,int> > unique; // (ip, maskLen)
  Net::hw_addr hw;
};

std::vector<iface_info> getInterfaces();

} // namespace NetIf

#endif
