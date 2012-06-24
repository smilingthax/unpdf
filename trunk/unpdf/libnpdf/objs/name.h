#ifndef UNPDF_OBJS_NAME_H
#define UNPDF_OBJS_NAME_H

#include "base.h"

namespace PDFTools {

  class Name : public Object {
  public:
    enum FreeT { DUP=0, TAKE, STATIC };
    Name(const char *name,FreeT ft=DUP);
    ~Name();
    void print(Output &out) const;
    Name *clone() const { return new Name(val,DUP); } // always copy

    const char *value() const { return val; }
  private:
    const char *val; // to getter + protected
    bool ours;

    Name(const Name&);
    const Name &operator=(const Name&);
  };

} // namespace PDFTools

#endif
