#ifndef UNPDF_OBJS_STRING_H
#define UNPDF_OBJS_STRING_H

#include "base.h"
#include <string>

namespace PDFTools {

  class Decrypt;
  class String : public Object {
  public:
    String();
    String(const char *str);
    String(const char *str,int len,bool as_hex=false);
    String(const std::string &str,const Decrypt *decrypt=NULL,bool as_hex=false);
    void print(Output &out) const;
    String *clone() const { return new String(*this); }

    const std::string &value() const { return val; }
  private:
    std::string val; // hehe
    bool as_hex;
  };

} // namespace PDFTools

#endif
