#ifndef UNPDF_CRYPT_SHA256_H
#define UNPDF_CRYPT_SHA256_H

#include <string>

namespace PDFTools {

  class SHA256 {
  public:
    enum { SIZE=32, BITS=SIZE*8 };

    SHA256();
    ~SHA256();
    void init();
    void update(const char *data,int len);
    void update(const std::string &str);
    void finish(char *hash); // >hash to be at least 32 bytes

    // allowed: hash==data
    static void sha256(char *hash,const char *data,int len);
  private:
    class SHA256_impl;
    SHA256_impl *impl;

    SHA256(const SHA256 &);
    const SHA256 &operator=(const SHA256 &);
  };

} // namespace PDFTools

#endif
