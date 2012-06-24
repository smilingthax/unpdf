#include "sha256.h"
#include <openssl/sha.h>
#include "exception.h"

namespace PDFTools {

struct SHA256::SHA256_impl {
  SHA256_CTX ctx;
};

SHA256::SHA256() // {{{
  : impl(new SHA256_impl)
{
}
// }}}

SHA256::~SHA256() // {{{
{
  delete impl;
}
// }}}

void SHA256::init() // {{{
{
  int res=SHA256_Init(&impl->ctx);
  if (!res) {
    throw UsrError("SHA256_Init failed");
  }
}
// }}}

void SHA256::update(const char *data,int len) // {{{
{
  int res=SHA256_Update(&impl->ctx,data,len);
  if (!res) {
    throw UsrError("SHA256_Update failed");
  }
}
// }}}

void SHA256::update(const std::string &str) // {{{
{
  int res=SHA256_Update(&impl->ctx,str.data(),str.size());
  if (!res) {
    throw UsrError("SHA256_Update failed");
  }
}
// }}}

void SHA256::finish(char *hash) // {{{
{
  int res=SHA256_Final((unsigned char*)hash,&impl->ctx);
  if (!res) {
    throw UsrError("SHA256_Final failed");
  }
}
// }}}

void SHA256::sha256(char *hash,const char *data,int len) // {{{
{
  unsigned char *res=::SHA256((const unsigned char *)data,len,(unsigned char *)hash);
  if (!res) {
    throw UsrError("SHA256 failed");
  }
}
// }}}

} // namespace PDFTools
