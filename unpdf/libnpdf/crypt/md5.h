#ifndef UNPDF_CRYPT_MD5_H
#define UNPDF_CRYPT_MD5_H

#include <string>

namespace PDFTools {

  class MD5 {
  public:
    enum { SIZE=16, BITS=SIZE*8 };

    MD5();
    ~MD5();
    void init();
    void update(const char *data,int len);
    void update(const std::string &str);
    void finish(char *hash); // >hash to be at least 16 bytes
    // void finish_b64(unsigned char *hash);

    // allowed: hash==data
    static void md5(char *hash,const char *data,int len);
    // static std::string md5_b64(const char *data,int len);

  private:
    class MD5_impl;
    MD5_impl *impl;

    MD5(const MD5 &);
    const MD5 &operator=(const MD5 &);
  };

} // namespace PDFTools

#endif
