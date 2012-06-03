#include <assert.h>
#include <string.h>
#include "pdfcrypt.h"
#include "exception.h"

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/rc4.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/err.h>

using namespace std;
using namespace PDFTools;

// class DecryptInput : public Input {
// ... key set up(vals,pwds,) , revision, ... obj_id, obj_gen
// };
// class EncryptOutput : public Output {  // THIS ONE IS BAD! esp. for string-objs
//   IDEA:  String::print() tests if Output.type==EncryptOutput.type, etc...
// ... key set up(vals,pwds,) , revision, ... obj_id, obj_gen
// };

// {{{ PDFTools::MD5
class PDFTools::MD5::MD5_impl {
public:
  MD5_CTX ctx;
};

PDFTools::MD5::MD5() : impl(new MD5_impl)
{
}

PDFTools::MD5::~MD5()
{
  delete impl;
}

void PDFTools::MD5::init()
{
  int res=MD5_Init(&impl->ctx);
  if (!res) {
    throw UsrError("MD5_Init failed");
  }
}

void PDFTools::MD5::update(const char *data,int len)
{
  int res=MD5_Update(&impl->ctx,data,len);
  if (!res) {
    throw UsrError("MD5_Update failed");
  }
}

void PDFTools::MD5::update(const string &str)
{
  int res=MD5_Update(&impl->ctx,str.data(),str.size());
  if (!res) {
    throw UsrError("MD5_Update failed");
  }
}

void PDFTools::MD5::finish(char *hash)
{
  int res=MD5_Final((unsigned char*)hash,&impl->ctx);
  if (!res) {
    throw UsrError("MD5_Final failed");
  }
}

void PDFTools::MD5::md5(char *hash,const char *data,int len)
{
  unsigned char *res=::MD5((const unsigned char *)data,len,(unsigned char *)hash);
  if (!res) {
    throw UsrError("MD5 failed");
  }
}
// }}}

// {{{ PDFTools::SHA256
class PDFTools::SHA256::SHA256_impl {
public:
  SHA256_CTX ctx;
};

PDFTools::SHA256::SHA256() : impl(new SHA256_impl)
{
}

PDFTools::SHA256::~SHA256()
{
  delete impl;
}

void PDFTools::SHA256::init()
{
  int res=SHA256_Init(&impl->ctx);
  if (!res) {
    throw UsrError("SHA256_Init failed");
  }
}

void PDFTools::SHA256::update(const char *data,int len)
{
  int res=SHA256_Update(&impl->ctx,data,len);
  if (!res) {
    throw UsrError("SHA256_Update failed");
  }
}

void PDFTools::SHA256::update(const string &str)
{
  int res=SHA256_Update(&impl->ctx,str.data(),str.size());
  if (!res) {
    throw UsrError("SHA256_Update failed");
  }
}

void PDFTools::SHA256::finish(char *hash)
{
  int res=SHA256_Final((unsigned char*)hash,&impl->ctx);
  if (!res) {
    throw UsrError("SHA256_Final failed");
  }
}

void PDFTools::SHA256::sha256(char *hash,const char *data,int len)
{
  unsigned char *res=::SHA256((const unsigned char *)data,len,(unsigned char *)hash);
  if (!res) {
    throw UsrError("SHA256 failed");
  }
}
// }}}

// {{{ PDFTools::RC4
class PDFTools::RC4::RC4_impl {
public:
  int m_startlen;
  unsigned char m_startkey[16];
  RC4_KEY m_key;
};

PDFTools::RC4::RC4() : impl(new RC4_impl)
{
  char buf[5]={0,0,0,0,0}; // 40 bits
  setkey(buf,5);
}

PDFTools::RC4::~RC4()
{
  delete impl;
}

PDFTools::RC4::RC4(const string &key) : impl(new RC4_impl)
{
  setkey(key.data(),key.size());
}

void PDFTools::RC4::setkey(const char *key,int len)
{
  assert( (len>0)&&(len<=16) );
  impl->m_startlen=len;
  memcpy(impl->m_startkey,key,len);
  restart();
}

void PDFTools::RC4::restart()
{
//  RC4_set_key(&impl->m_key,impl->m_startlen,(const unsigned char *)impl->m_startkey);
  RC4_set_key(&impl->m_key,impl->m_startlen,impl->m_startkey);
}

void PDFTools::RC4::crypt(char *dst,const char *src,int len)
{
  assert(len>=0);
  ::RC4(&impl->m_key,len,(const unsigned char *)src,(unsigned char *)dst);
}
// }}}

// {{{ PDFTools::AESCBC
class PDFTools::AESCBC::AES_impl {
public:
  AES_KEY m_key;
};

PDFTools::AESCBC::AESCBC() : impl(new AES_impl)
{
  char buf[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 128 bits
  setkey(buf,16,true);
}

PDFTools::AESCBC::~AESCBC()
{
  delete impl;
}

PDFTools::AESCBC::AESCBC(const string &key,bool encrypt) : impl(new AES_impl)
{
  setkey(key.data(),key.size(),encrypt);
}

void PDFTools::AESCBC::setkey(const char *key,int len,bool encrypt)
{
  assert( (len==16)||(len==32) );
  if (encrypt) {
    AES_set_encrypt_key((const unsigned char *)key,len*8,&impl->m_key);
  } else {
    AES_set_decrypt_key((const unsigned char *)key,len*8,&impl->m_key);
  }
}

// TODO? allow iv=NULL (i.e. keep track internally?)
void PDFTools::AESCBC::encrypt(char *dst,const char *src,int len,char *iv)
{
  AES_cbc_encrypt((const unsigned char *)src,(unsigned char *)dst,len,&impl->m_key,(unsigned char *)iv,AES_ENCRYPT);
}

void PDFTools::AESCBC::decrypt(char *dst,const char *src,int len,char *iv)
{
  AES_cbc_encrypt((const unsigned char *)src,(unsigned char *)dst,len,&impl->m_key,(unsigned char *)iv,AES_DECRYPT);
}
// }}}

// {{{ PDFTools::RAND
std::string PDFTools::RAND::get(int len)
{
  string ret;
  ret.resize(len);

  if (RAND_bytes((unsigned char *)&ret[0],len)==0) {
    if (RAND_pseudo_bytes((unsigned char *)&ret[0],len)==0) {
      throw UsrError("PRNG failed: %ld",ERR_get_error());
    }
    fprintf(stderr,"Warning: non-secure PRNG used\n");
  }

  return ret;
}
// }}}
