#ifndef UNPDF_OBJS_NAME_H
#define UNPDF_OBJS_NAME_H

#include "base.h"

namespace PDFTools {

  // TODO: use std::move instead
  struct Name_ref { // helper,inline
    Name_ref() : ptr(0),ours(false) {}      // empty. special case for Dict::getName
    Name_ref(const char *ptr,bool take) : ptr(ptr),ours(take) {}

    const char *ptr;
    bool ours;
  };

  class Name : public Object {
  public:
    enum FreeT { DUP=0, TAKE, STATIC };
    Name(const char *name,FreeT ft=DUP);
    ~Name();

    Name(Name_ref ref) : val(ref.ptr),ours(ref.ours) {}
    operator Name_ref() {
      const char *ret=val;
      val=0;
      return Name_ref(ret,ours);
    }

    void print(Output &out) const;
    Name *clone() const {
      if (empty()) return NULL; // not supported
      return new Name(val,DUP); // always copy
    }

    bool empty() const { return (val==NULL); }
    const char *value() const { return val; }

    // special functions
    const char *release() {
      if (!ours) return NULL;
      ours=false;
      return val;
    }
  private:
    const char *val; // to getter + protected
    bool ours;

    Name(const Name&);
    const Name &operator=(const Name&);
  };

} // namespace PDFTools

#endif
