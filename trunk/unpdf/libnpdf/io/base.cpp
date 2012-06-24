#include "base.h"
#include <assert.h>
#include <stdio.h>
#include "tools.h"

namespace PDFTools {

std::string Input::gets(int len) // {{{
{
  int rlen;
#define BUFLEN 512
  char buf[BUFLEN];
  std::string ret;

  if (len<0) { // read to newline
    // TODO: optimize
    char c;
    for (;len>0;len--) {
      rlen=read(&c,1);
      if (rlen!=1) {
        return ret;
      }
      ret.push_back(c);
    }
    return ret;
  }
#if 0
  do {
    const int l=min(len,BUFLEN);
    rlen=read(buf,l);
    if (rlen<0) {
      return ret;
    }
    len-=rlen;
    ret.append(buf,rlen);
  } while ( (len==0)||(rlen<l) );
#else
  ret.resize(len);
  rlen=read(&ret[0],len);
  ret.resize(rlen);
#endif
  return ret;
}
// }}}

// {{{ Output
void Output::printf(const char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  vprintf(fmt,ap);
  va_end(ap);
}

void Output::vprintf(const char *fmt,va_list ap)
{
  puts(s_vsprintf(fmt,ap).c_str());
}

void Output::puts(const char *str)
{
  write(str,-1);
}

void Output::put(const char c)
{
  write(&c,1);
}

void Output::flush()
{
}
// }}}

} // namespace PDFTools
