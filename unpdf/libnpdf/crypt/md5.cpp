#include "md5.h"
#include <openssl/md5.h>
#include "exception.h"

namespace PDFTools {

struct MD5::MD5_impl {
  MD5_CTX ctx;
};

MD5::MD5() // {{{
  : impl(new MD5_impl)
{
}
// }}}

MD5::~MD5() // {{{
{
  delete impl;
}
// }}}

void MD5::init() // {{{
{
  int res=MD5_Init(&impl->ctx);
  if (!res) {
    throw UsrError("MD5_Init failed");
  }
}
// }}}

void MD5::update(const char *data,int len) // {{{
{
  int res=MD5_Update(&impl->ctx,data,len);
  if (!res) {
    throw UsrError("MD5_Update failed");
  }
}
// }}}

void MD5::update(const std::string &str) // {{{
{
  int res=MD5_Update(&impl->ctx,str.data(),str.size());
  if (!res) {
    throw UsrError("MD5_Update failed");
  }
}
// }}}

void MD5::finish(char *hash) // {{{
{
  int res=MD5_Final((unsigned char*)hash,&impl->ctx);
  if (!res) {
    throw UsrError("MD5_Final failed");
  }
}
// }}}

void MD5::md5(char *hash,const char *data,int len) // {{{
{
  unsigned char *res=::MD5((const unsigned char *)data,len,(unsigned char *)hash);
  if (!res) {
    throw UsrError("MD5 failed");
  }
}
// }}}

} // namespace PDFTools
