#ifndef NET_H_
#define NET_H_

#include <stdexcept>
#include <string>
#include <vector>

#ifdef WIN32     // Win32: must link against ws2_32
#include <ws2tcpip.h>
typedef u_short sa_family_t;
#else
#include <netinet/in.h>  // for sockaddr_in
#endif

namespace Net {

class Error : public std::runtime_error {
public:
  Error(const char *str);
  Error(const char *str, int err);
  Error(const char *str, const std::string &errstr, int err);
  int err;
};

struct ip_addr {
//  ip_addr();
  ip_addr(const char *str); // i.e. "1.2.3.4"
  ip_addr(const struct sockaddr *sa, socklen_t len); // NOTE: includes scope
  ip_addr(const struct in_addr *in);
  ip_addr(const struct in6_addr *in6);

  sa_family_t type() const {
    return ss.ss_family;
  }

  const struct sockaddr *get() const {
    return (struct sockaddr *)&ss;
  }

  socklen_t size() const {
    switch (type()) {
    case AF_INET:
      return sizeof(struct sockaddr_in);
    case AF_INET6:
      return sizeof(struct sockaddr_in6);
    }
    throw new Error("bad type");
  }

  std::string str() const;

private:
  struct sockaddr_storage ss;
};

struct hw_addr {
  bool empty() const {
    return addr.empty();
  }

  std::string str() const;

  std::vector<unsigned char> addr;
};

#ifdef WIN32
namespace detail {
class wsa {
public:
  static void ensure();
  static void destroy();
private:
  static bool initialized;
};
} // namespace detail
#endif

} // namespace Net

#endif
