#include "aescbc.h"
#include <assert.h>
#include <openssl/aes.h>

namespace PDFTools {

struct AESCBC::AES_impl {
  AES_KEY m_key;
};

AESCBC::AESCBC() // {{{
  : impl(new AES_impl)
{
  char buf[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 128 bits
  setkey(buf,16,true);
}
// }}}

AESCBC::~AESCBC() // {{{
{
  delete impl;
}
// }}}

AESCBC::AESCBC(const std::string &key,bool encrypt) // {{{
  : impl(new AES_impl)
{
  setkey(key.data(),key.size(),encrypt);
}
// }}}

void AESCBC::setkey(const char *key,int len,bool encrypt) // {{{
{
  assert( (len==16)||(len==32) );
  if (encrypt) {
    AES_set_encrypt_key((const unsigned char *)key,len*8,&impl->m_key);
  } else {
    AES_set_decrypt_key((const unsigned char *)key,len*8,&impl->m_key);
  }
}
// }}}

// TODO? allow iv=NULL (i.e. keep track internally?)
void AESCBC::encrypt(char *dst,const char *src,int len,char *iv) // {{{
{
  AES_cbc_encrypt((const unsigned char *)src,(unsigned char *)dst,len,&impl->m_key,(unsigned char *)iv,AES_ENCRYPT);
}
// }}}

void AESCBC::decrypt(char *dst,const char *src,int len,char *iv) // {{{
{
  AES_cbc_encrypt((const unsigned char *)src,(unsigned char *)dst,len,&impl->m_key,(unsigned char *)iv,AES_DECRYPT);
}
// }}}

} // namespace PDFTools
