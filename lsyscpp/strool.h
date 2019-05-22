#ifndef STROOL_H_
#define STROOL_H_

#include <string.h>

// meant for c++11 lambdas ...

namespace Str {

// NOTE: will include empty strings

template <typename Fn>  // fn(const char *start, size_t len)
void explode(const char *str, char delim, Fn fn) // {{{
{
  const char *next=strchr(str,delim);
  while (next) {
    fn(str,next-str);
    str=next;
    next=strchr(str,delim);
  }
  if (*str) {
    fn(str, strlen(str));
  }
}
// }}}

} // namespace

#endif
