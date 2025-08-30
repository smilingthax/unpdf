#ifndef UNPDF_SECURTY_AESCRYPT_H
#define UNPDF_SECURTY_AESCRYPT_H

#include "../io/crypt.h"
#include <string>

namespace PDFTools {

  class StandardAESDecrypt : public Decrypt { // partially symmetric
  public:
    StandardAESDecrypt(const std::string &key);

    void operator()(std::string &dst,const std::string &src) const;
    Input *getInput(Input &read_from) const;
  private:
    std::string key;
    class AESInput;
  };

  class StandardAESEncrypt : public Encrypt { // partially symmetric
  public:
    StandardAESEncrypt(const std::string &key,const std::string &iv=std::string());

    void operator()(std::string &dst,const std::string &src) const { operator()(dst,src,std::string()); }
    void operator()(std::string &dst,const std::string &src,const std::string &iv) const;
    Output *getOutput(Output &write_to) const;
  private:
    std::string key,useiv;
    class AESOutput;
  };

} // namespace PDFTools

#endif
