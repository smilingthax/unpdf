#include "net.h"
#include <assert.h>
#include <string.h>

#ifndef WIN32
#include <netdb.h>
#endif

#if __cplusplus>=201103L
using std::to_string;
#else
#include <stdio.h>
static std::string to_string(int num) {
  char buf[20];
  snprintf(buf, 20, "%d", num);
  return buf;
}
#endif

namespace Net {

Error::Error(const char *str)
  : std::runtime_error(std::string(str)), err(0)
{
}

Error::Error(const char *str, int err)
  : std::runtime_error(std::string(str) + ": (" + to_string(err) + ")"), err(err)
{
}

Error::Error(const char *str, const std::string &errstr, int err)
  : std::runtime_error(std::string(str) + ": " + errstr + " (" + to_string(err) + ")"), err(err)
{
}

#ifdef WIN32
void detail::wsa::ensure() // {{{
{
  if (!initialized) {
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
      throw Error("WSAStartup failed:", err);
    }
    initialized = true;
  }
}
// }}}

void detail::wsa::destroy() { // {{{
  if (initialized) {
    WSACleanup();
    initialized=false;
  }
}
// }}}

bool detail::wsa::initialized = false;
#define ENSURE_WSA detail::wsa::ensure()

#else
#define ENSURE_WSA
#endif

// only allows AF_INET or AF_INET6, otherwise throws
static void _ipaddr_set_sockaddr(sa_family_t family, struct sockaddr *dst, const struct sockaddr *src, socklen_t srclen) // {{{
{
  switch (family) {
  case AF_INET:
    assert(sizeof(struct sockaddr_in) == srclen);
    memcpy(dst, src, sizeof(struct sockaddr_in));
    break;
  case AF_INET6:
    assert(sizeof(struct sockaddr_in6) == srclen);
    memcpy(dst, src, sizeof(struct sockaddr_in6));
    break;
  default:
    char buf[50];
    snprintf(buf, 50, "_ipaddr_set_sockaddr: unexpected AF_ %d\n", family);
    throw Error(buf);
  }
}
// }}}

static bool parse_ip(const char *str, struct sockaddr *sa) // {{{
{
  // assert(dstdata);
  struct addrinfo hints = {0};
  hints.ai_socktype = SOCK_STREAM; // we don't want them duped for STREAM, DGRAM, RAW, ...
  hints.ai_flags = AI_NUMERICHOST;

  struct addrinfo *ai;
  int res = getaddrinfo(str, NULL, &hints, &ai);
  if (res != 0) {
    throw Error("getaddrinfo failed", gai_strerror(res), res);
  }
  if (!ai) {
    freeaddrinfo(ai);
    return false;
  }
  try {
    _ipaddr_set_sockaddr(ai->ai_family, sa, ai->ai_addr, ai->ai_addrlen);
    assert(!ai->ai_next);
  } catch (...) {
    freeaddrinfo(ai);
    throw;
  }
  freeaddrinfo(ai);
  return true;
}
// }}}

static std::string _ip_addr_str(const struct sockaddr *sa, socklen_t len) // {{{
{
  char addr[INET6_ADDRSTRLEN];
  int res = getnameinfo(sa, len, addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
  if (res != 0) {
    throw Error("getnameinfo failed", gai_strerror(res), res);
  }
  return addr;
}
// }}}

// ip_addr::ip_addr()    ???

ip_addr::ip_addr(const char *str) // {{{
{
  if (!str) {
    throw Error("str == NULL");
  }
  ENSURE_WSA;

  if (!parse_ip(str, (struct sockaddr *)&ss)) {
    throw Error("parse_ip() returned no data");
  }
}
// }}}

ip_addr::ip_addr(const struct sockaddr *sa, socklen_t len) // {{{
{
  if (!sa) {
    throw Error("sa == NULL");
  }
  ENSURE_WSA;

  _ipaddr_set_sockaddr(sa->sa_family, (struct sockaddr *)&ss, sa, len);
}
// }}}

ip_addr::ip_addr(const struct in_addr *in) // {{{
{
  if (!in) {
    throw Error("in == NULL");
  }
  ENSURE_WSA;

  struct sockaddr_in sin = {0};
  sin.sin_family = AF_INET;
  sin.sin_addr = *in;
  memcpy(&ss, &sin, sizeof(sin));
}
// }}}

ip_addr::ip_addr(const struct in6_addr *in6) // {{{
{
  if (!in6) {
    throw Error("in6 == NULL");
  }
  ENSURE_WSA;

  struct sockaddr_in6 sin6 = {0};
  sin6.sin6_family = AF_INET6;
  sin6.sin6_addr = *in6;
  memcpy(&ss, &sin6, sizeof(sin6));
}
// }}}

std::string ip_addr::str() const // {{{
{
  return _ip_addr_str(get(), size());
}
// }}}

std::string hw_addr::str() const // {{{
{
  std::string ret;
  const size_t len = addr.size();
  if (!len) {
    return ret;
  }
  ret.reserve(len*3);
  char buf[5];
  int res = snprintf(buf, 5, "%02x", addr[0]);
  ret.assign(buf, res);
  for (size_t i=1; i<len; i++) {
    res = snprintf(buf, 5, ":%02x", addr[i]);
    ret.append(buf, res);
  }
  return ret;
}
// }}}

} // namespace Net

