#ifndef UNPDF_SECURITY_RC4CRYPT_H
#define UNPDF_SECURITY_RC4CRYPT_H

#include "../io/crypt.h"

namespace PDFTools {

  class StandardRC4Crypt : public Decrypt,public Encrypt { // fully symmetric
  public:
    StandardRC4Crypt(const std::string &objkey);
    
    void operator()(std::string &dst,const std::string &src) const;
    Input *getInput(Input &read_from) const;
    Output *getOutput(Output &write_to) const;
  private:
    std::string objkey;
    class RC4Input;
    class RC4Output;
  };

} // namespace PDFTools

#endif
