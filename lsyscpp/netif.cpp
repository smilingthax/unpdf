#include "netif.h"
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#define ENSURE_WSA Net::detail::wsa::ensure()

#else
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <net/if.h>  // IFF_UP, ...
#include <map>
#ifdef __linux__
  #include <netpacket/packet.h>  // sockaddr_ll
#endif
#ifdef __APPLE__
  #include <net/if_dl.h>  // sockaddr_dl
#endif
#define ENSURE_WSA

#endif

namespace NetIf {

using Net::Error;
using Net::ip_addr;

static void throw_errno(const char *str) // {{{
{
#ifdef WIN32
  throw Error(str, WSAGetLastError());
#else
  throw Error(str, errno);
#endif
}
// }}}

std::string getHostName() // {{{
{
  ENSURE_WSA;
  char buf[256];
  if (gethostname(buf, sizeof(buf)) != 0) {
    throw_errno("gethostname failed");
  }
  return buf;
}
// }}}

class SocketPtr {
public:
  SocketPtr(int fd) : fd(fd) {}
  ~SocketPtr();

  int operator*() const { return fd; }
  int get() const { return fd; }
  int release() {
    int ret = fd;
    fd = -1;
    return ret;
  }

private:
  int fd;
  SocketPtr(const SocketPtr &);
  SocketPtr &operator=(const SocketPtr &);
};

SocketPtr::~SocketPtr() // {{{
{
  if (fd < 0) {
    return;
  }
#ifdef WIN32
  closesocket(fd);
#else
  close(fd);
#endif
}
// }}}

// a.k.a. getDefaultLocalAddress
ip_addr getDefaultSource4() // {{{
{
  ENSURE_WSA;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    throw_errno("socket failed");
  }
  SocketPtr sock(fd);  // TODO? socket class?

  struct sockaddr_in sin = {0};
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0x08080808; // = htonl(8.8.8.8)
  sin.sin_port = htons(53); // dns
  // NOTE: will not send anything
  if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    throw_errno("connect failed");
  }

  socklen_t len = sizeof(sin);
  if (getsockname(fd, (struct sockaddr *)&sin, &len) < 0) {
    throw_errno("getsockname failed");
  }
  sin.sin_port = 0; // hack...
  return ip_addr((const struct sockaddr *)&sin, len);
}
// }}}

#ifdef WIN32
static std::string wstr_to_utf8(const wchar_t *wstr, int wlen=-1) // {{{
{
  // assert(wstr);
  if (wlen == -1) {
    wlen = wcslen(wstr);
  }
  int len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, NULL, 0, NULL, NULL);
  std::string ret;
  ret.resize(len);
  WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, &ret[0], len, NULL, NULL);
  return ret;
}
// }}}

static void throw_WinError(const char *str, DWORD err) // {{{
{
  LPTSTR buf = NULL;
  DWORD len = FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    err,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),   // Default language
    buf, 0,
    NULL
  );
  if (len > 0) {
    std::string msg(buf, len);
    LocalFree(buf);
    throw Error(str, msg, err);
  }
  throw Error(str, err);
}
// }}}

// alen is number of bits!
// NOTE: also checks that trailing bits of b are zeros
bool _ip4_prefix_eq(const struct in_addr *a, const struct in_addr *b, int alen) // {{{
{
  // _truncate_ip(a, len) == b
  if (alen > 31) {
    return a->s_addr == b->s_addr;
  } else if (alen <= 0) {
    return b->s_addr == INADDR_ANY; // 0
  }
  const unsigned int mask = 0xffffffff << (32-alen);
  return (a->s_addr & htonl(mask)) == b->s_addr;
}
// }}}

bool _ip6_prefix_eq(const struct in6_addr *a, const struct in6_addr *b, int alen) // {{{
{
  if (alen > 127) {
    return memcmp(&a->s6_addr, &b->s6_addr, 16) == 0;
  } else if (alen <= 0) {
    return memcmp(&in6addr_any.s6_addr, &b->s6_addr, 16) == 0;
  }
  const int bnum = alen / 8, bits = alen % 8;
  if (memcmp(&a->s6_addr, &b->s6_addr, bnum) != 0) {
    return false;
  }
  const unsigned char mask = 0xff << (8-bits);
  if ((a->s6_addr[bnum] & mask) != b->s6_addr[bnum]) {
    return false;
  }
  // assert(bnum < 16);
  return memcmp(&in6addr_any.s6_addr[bnum + 1], &b->s6_addr[bnum + 1], 16-bnum-1) == 0;
}
// }}}

#if _WIN32_WINNT<0x0600
// idea from webrtc    // HACK ... host prefix shall *not* be best
// returns -1, when none found
static std::pair<ip_addr, int> getBestPrefix(PIP_ADAPTER_PREFIX list, const ip_addr &ip) // {{{
{
  int bestLen = -1;
  switch (ip.type()) {
  case AF_INET: {
    struct in_addr in, best;
    for (; list; list=list->Next) {
//      if ((int)list->PrefixLength <= bestLen) {
      if ((int)list->PrefixLength <= bestLen && bestLen!=32) {
        continue;
      } else if (!list->Address.lpSockaddr ||
                 list->Address.lpSockaddr->sa_family != AF_INET) {
        continue;
      }
      assert(sizeof(struct sockaddr_in) == list->Address.iSockaddrLength);
      memcpy(&in, &((const struct sockaddr_in *)list->Address.lpSockaddr)->sin_addr.s_addr, sizeof(in));
      if (_ip4_prefix_eq(&((const struct sockaddr_in *)ip.get())->sin_addr, &in, list->PrefixLength)) {
if (list->PrefixLength == 32 && bestLen >= 0) continue;
        best = in;
        bestLen = list->PrefixLength;
      }
    }
    return std::make_pair(ip_addr(&best), bestLen);
  }

  case AF_INET6: {
    struct in6_addr in6, best;
    for (; list; list=list->Next) {
//      if ((int)list->PrefixLength <= bestLen) {
      if ((int)list->PrefixLength <= bestLen && bestLen!=128) {
        continue;
      } else if (!list->Address.lpSockaddr ||
                 list->Address.lpSockaddr->sa_family != AF_INET6) {
        continue;
      }
      assert(sizeof(struct sockaddr_in6) == list->Address.iSockaddrLength);
      memcpy(&in6, &((const struct sockaddr_in6 *)list->Address.lpSockaddr)->sin6_addr.s6_addr, sizeof(in6));
      if (_ip6_prefix_eq(&((const struct sockaddr_in6 *)ip.get())->sin6_addr, &in6, list->PrefixLength)) {
if (list->PrefixLength == 128 && bestLen >= 0) continue;
        best = in6;
        bestLen = list->PrefixLength;
      }
    }
    return std::make_pair(ip_addr(&best), bestLen);
  }

  default:
    throw Error("bad ip type");
  }
}
// }}}
#endif

static std::vector<iface_info> _w32_getInterfaces() // {{{
{
  static const int MAX_TRIES = 3;
  std::vector<iface_info> ret;

  PIP_ADAPTER_ADDRESSES pAddrs = NULL;
  ULONG size = 15*1024; // start with 15kB
  DWORD res;
  for (int tries=0; tries<MAX_TRIES; tries++) { // tries: when net config changes between calls ...
    pAddrs = (PIP_ADAPTER_ADDRESSES)malloc(size);
    if (!pAddrs) {
      throw std::bad_alloc();
    }
    res = GetAdaptersAddresses(
      AF_UNSPEC,
#if _WIN32_WINNT>=0x0600
      GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_FRIENDLY_NAME,
#else
      GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_FRIENDLY_NAME,
#endif
      NULL,
      pAddrs,
      &size
    );
    if (res != ERROR_BUFFER_OVERFLOW) {
      break;
    }
    free(pAddrs);
    pAddrs = NULL;
  }
  if (res != NO_ERROR) {
    free(pAddrs);
    fprintf(stderr, "GetAdaptersAddresses failed: %ld\n", res);
    if (res == ERROR_NO_DATA) {
      return ret; // empty
    }
    throw_WinError("GetAdapterAddresses failed", res);
  }

  try {
    for (PIP_ADAPTER_ADDRESSES cur=pAddrs; cur; cur=cur->Next) {
      if (cur->OperStatus != IfOperStatusUp) {
        continue;
      }
      // if (cur->IfType == IF_TYPE_SOFTWARE_LOOPBACK) { continue; }
      struct iface_info info;

      for (PIP_ADAPTER_UNICAST_ADDRESS addr=cur->FirstUnicastAddress; addr; addr=addr->Next) {
        if (addr->Flags & IP_ADAPTER_ADDRESS_TRANSIENT) {
          continue;
        }
        ip_addr ip(addr->Address.lpSockaddr, addr->Address.iSockaddrLength);

#if _WIN32_WINNT>=0x0600
        if (addr->OnLinkPrefixLength == 255) {
          continue;
//          throw Error("illegal prefix");  // ?
        }
        const int pfxLen = addr->OnLinkPrefixLength; // possibly 0 ..??
#else
        std::pair<ip_addr, int> pfx = getBestPrefix(cur->FirstPrefix, ip);
        if (pfx.second < 0) {
          continue;  // TODO?
//          throw Error("no prefix found");
        }
        const int pfxLen = pfx.second; // possibly 0 ...
#endif
#if __cplusplus>=201103L
        info.unique.emplace_back(std::move(ip), pfxLen);
#else
        info.unique.push_back(std::make_pair(ip, pfxLen));
#endif
      }
      if (info.unique.empty()) {
        continue;
      }
      info.name = wstr_to_utf8(cur->Description);

      info.hw.addr.resize(cur->PhysicalAddressLength);
      memcpy(&info.hw.addr[0], cur->PhysicalAddress, cur->PhysicalAddressLength);

#if __cplusplus>=201103L
      ret.emplace_back(std::move(info));
#else
      ret.push_back(info);
#endif
    }
  } catch (...) {
    free(pAddrs);
    throw;
  }
  free(pAddrs);
  return ret;
}
// }}}

#else
// returns -1 when not left-contiguous, -2 on nullptr
static int mask4_to_bits(const struct sockaddr_in *sin) // {{{
{
  if (!sin) {
    return -2;
  }
  const uint32_t val = ntohl(sin->sin_addr.s_addr);
  if (!val) {
    return 0;
  }
  const uint32_t msb = -val;
  if (msb & ~val) { // msb not a power of two?
    return -1;
  }
  // asert(msb != 0);
  return __builtin_clz(msb) + 1;
  // return 32 - __builtin_ctz(msb);
}
// }}}

// returns -1 when not left-contiguous, -2 on nullptr
static int mask6_to_bits(const struct sockaddr_in6 *sin6) // {{{
{
  if (!sin6) {
    return -2;
  }
  for (int i=0; i<16; i++) {
    const unsigned char c = sin6->sin6_addr.s6_addr[i];
    if (c != 0xff) {
      int ret = i*8;
      if (c) { // basically 8-ctz(c)
        const unsigned char msb = -c;
        if (msb & ~c) { // msb not a power of two?
          return -1;
        }
        ret += 8;
        ret -= ((msb&0xaa) != 0) | (((msb&0xcc) != 0) << 1) | (((msb&0xf0) != 0) << 2);
      }
      for (i++; i<16; i++) {
        if (sin6->sin6_addr.s6_addr[i] != 0) {
          return -1;
        }
      }
      return ret;
    }
  }
  return 128;
}
// }}}

  // TODO strip loopback ?
static std::vector<iface_info> _px_getInterfaces() // {{{
{
  struct ifaddrs *ifaddrs;
  if (getifaddrs(&ifaddrs) == -1) {
    throw Error("getifaddrs failed", errno);
  }

  std::map<std::string, iface_info> group; // c++11? unordered_map ?
  try {
    for (struct ifaddrs *ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
      // cf. netdevice(7) / SIOCGIFFLAGS
      if ((ifa->ifa_flags & IFF_UP) == 0 ||
          !ifa->ifa_addr) {
        continue;
      }
//      if (ifa->ifa_flags & IFF_LOOPBACK) { continue; }

#if __cpp_lib_map_try_emplace >= 201411L
      auto it_inserted = group.try_emplace(ifa->ifa_name);
#elif __cplusplus>=201103L
      auto it_inserted = group.emplace(std::string{ifa->ifa_name}, iface_info{});
#else
      std::pair<std::map<std::string, iface_info>::iterator, bool> it_inserted = group.insert(std::make_pair(std::string(ifa->ifa_name), iface_info()));
#endif
      struct iface_info &info = it_inserted.first->second;
      if (it_inserted.second) {
        info.name = it_inserted.first->first;
      }

      switch (ifa->ifa_addr->sa_family) {
      case AF_INET: {
        if (!ifa->ifa_netmask) {
          continue;
        } else if (ifa->ifa_netmask->sa_family != AF_INET) {
          throw Error("mask is not AF_INET");
        }
        const int pfx = mask4_to_bits((const struct sockaddr_in *)ifa->ifa_netmask);
        if (pfx < 0) {
          throw Error("non-contiguous netmask");
        }
        ip_addr ip(ifa->ifa_addr, sizeof(struct sockaddr_in));
#if __cplusplus>=201103L
        info.unique.emplace_back(std::move(ip), pfx);
#else
        info.unique.push_back(std::make_pair(ip, pfx));
#endif
        break;
      }

      case AF_INET6: {
        if (!ifa->ifa_netmask) {
          continue;
        } else if (ifa->ifa_netmask->sa_family != AF_INET6) {
          throw Error("mask is not AF_INET6");
        }
        const int pfx = mask6_to_bits((const struct sockaddr_in6 *)ifa->ifa_netmask);
        if (pfx < 0) {
          throw Error("non-contiguous netmask");
        }
        ip_addr ip(ifa->ifa_addr, sizeof(struct sockaddr_in6));
#if __cplusplus>=201103L
        info.unique.emplace_back(std::move(ip), pfx);
#else
        info.unique.push_back(std::make_pair(ip, pfx));
#endif
        break;
      }

#ifdef __linux__
      case AF_PACKET: {
        struct sockaddr_ll *ll = (struct sockaddr_ll *)ifa->ifa_addr;
        // NOTE: overwrites  // TODO?
        info.hw.addr.resize(ll->sll_halen);
        memcpy(&info.hw.addr[0], ll->sll_addr, ll->sll_halen);
        break;
      }
#endif
#ifdef __APPLE__
      case AF_LINK: {
        struct sockaddr_dl *dl = (struct sockaddr_dl *)ifa->ifa_addr;
        // NOTE: overwrites  // TODO?
        info.hw.addr.resize(dl->sdl_alen);
        memcpy(&info.hw.addr[0], LLADDR(dl), dl->sdl_alen);
        break;
      }
#endif
      default:
        continue;
      }
    }
  } catch (...) {
    freeifaddrs(ifaddrs);
    throw;
  }
  freeifaddrs(ifaddrs);

  std::vector<iface_info> ret;
#if __cplusplus>=201103L
  for (auto &grp : group) {
    if (grp.second.unique.empty()) {
      continue;
    }
    ret.push_back(std::move(grp.second));
  }
#else
  for (std::map<std::string, iface_info>::const_iterator it=group.begin(), end=group.end(); it!=end; ++it) {
    if (it->second.unique.empty()) {
      continue;
    }
    ret.push_back(it->second);
  }
#endif
  return ret;
}
// }}}

#endif

std::vector<iface_info> getInterfaces() // {{{
{
#ifdef WIN32
  return _w32_getInterfaces();
#else
  return _px_getInterfaces();
#endif
}
// }}}

} // namespace NetIf

