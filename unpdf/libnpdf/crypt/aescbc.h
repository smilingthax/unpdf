#ifndef UNPDF_CRYPT_AESCBC_H
#define UNPDF_CRYPT_AESCBC_H

#include <string>

namespace PDFTools {

  class AESCBC {
  public:
    AESCBC(); // default: encrypt
    AESCBC(const std::string &key,bool encrypt);
    ~AESCBC();
    // 16 or 32 byte >key
    void setkey(const char *key,int len,bool encrypt);

    // 16 byte >iv, will be updated,  >len to be multiple of 16
    void encrypt(char *dst,const char *src,int len,char *iv);
    void decrypt(char *dst,const char *src,int len,char *iv);
       // dst==src is allowed
  private:
    class AES_impl;
    AES_impl *impl;

    AESCBC(const AESCBC &);
    const AESCBC &operator=(const AESCBC &);
  };

} // namespace PDFTools

#endif
