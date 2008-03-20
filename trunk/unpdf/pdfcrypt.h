#ifndef _PDFCRYPT_H
#define _PDFCRYPT_H

#include <string>

namespace PDFTools {
  class MD5 {
  public:
    MD5();
    ~MD5();
    void init();
    void update(const char *data,int len);
    void update(const std::string &str);
    void finish(char *hash); // >hash to be at least 16 bytes
    // void finish_b64(unsigned char *hash);

    static void md5(char *hash,const char *data,int len); // allowed: hash==data
    // static std::string md5_b64(const char *data,int len);

  private:
    class MD5_impl;
    MD5_impl *impl;

    MD5(const MD5 &);
    const MD5 &operator=(const MD5 &);
  };
  class RC4 {
  public:
    RC4();
    RC4(const std::string &key);
    ~RC4();
    void setkey(const char *key,int len);

    void crypt(char *dst,const char *src,int len); // decrypt==encrypt
       // dst==src is allowed
  private:
    class RC4_impl;
    RC4_impl *impl;

    RC4(const RC4 &);
    const RC4 &operator=(const RC4 &);
  };
  class AESCBC {
  public:
    AESCBC(); // default: encrypt
    AESCBC(const std::string &key,bool encrypt);
    ~AESCBC();
    // 16 byte >iv
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
  class RAND {
  public:
    static std::string get(int len);
  };
};

#endif
