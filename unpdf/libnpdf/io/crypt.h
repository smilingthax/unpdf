#ifndef UNPDF_IO_CRYPT_H
#define UNPDF_IO_CRYPT_H

#include <string>

namespace PDFTools {

  // ATTENTION: {Crypto}Output::flush() will restart crypto!

  class Input;
  class Decrypt {
  public:
    virtual ~Decrypt() {}

    virtual void operator()(std::string &dst,const std::string &src) const=0;
    virtual Input *getInput(Input &read_from) const=0;
  };

  class Output;
  class Encrypt {
  public:
    virtual ~Encrypt() {}

    virtual void operator()(std::string &dst,const std::string &src) const=0;
    virtual Output *getOutput(Output &write_to) const=0;
  };

};

#endif
