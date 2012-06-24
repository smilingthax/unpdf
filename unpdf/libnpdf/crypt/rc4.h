#ifndef UNPDF_CRYPT_RC4_H
#define UNPDF_CRYPT_RC4_H

#include <string>

namespace PDFTools {

  class RC4 {
  public:
    RC4();
    RC4(const std::string &key);
    ~RC4();
    void setkey(const char *key,int len);
    void restart();

    void crypt(char *dst,const char *src,int len); // decrypt==encrypt
       // dst==src is allowed
  private:
    class RC4_impl;
    RC4_impl *impl;

    RC4(const RC4 &);
    const RC4 &operator=(const RC4 &);
  };

} // namespace PDFTools

#endif
