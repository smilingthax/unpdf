#include "rc4.h"
#include <assert.h>
#include <string.h>
#include <openssl/rc4.h>

namespace PDFTools {

struct RC4::RC4_impl { 
  int m_startlen;
  unsigned char m_startkey[16];
  RC4_KEY m_key;
};

RC4::RC4() // {{{
  : impl(new RC4_impl)
{
  char buf[5]={0,0,0,0,0}; // 40 bits
  setkey(buf,5);
}
// }}}

RC4::~RC4() // {{{
{
  delete impl;
}
// }}}

RC4::RC4(const std::string &key) // {{{
  : impl(new RC4_impl)
{
  setkey(key.data(),key.size());
}
// }}}

void RC4::setkey(const char *key,int len) // {{{
{
  assert( (len>0)&&(len<=16) );
  impl->m_startlen=len;
  memcpy(impl->m_startkey,key,len);
  restart();
}
// }}}

void RC4::restart() // {{{
{
//  RC4_set_key(&impl->m_key,impl->m_startlen,(const unsigned char *)impl->m_startkey);
  RC4_set_key(&impl->m_key,impl->m_startlen,impl->m_startkey);
}
// }}}

void RC4::crypt(char *dst,const char *src,int len) // {{{
{
  assert(len>=0);
  ::RC4(&impl->m_key,len,(const unsigned char *)src,(unsigned char *)dst);
}
// }}}

} // namespace PDFTools
