#ifndef UNPDF_OBJS_REF_H
#define UNPDF_OBJS_REF_H

#include "base.h"

namespace PDFTools {

  class Ref : public Object {
  public:
    Ref(int ref=0,int gen=0) : ref(ref),gen(gen) {}
    Ref(const Ref &r) : ref(r.ref),gen(r.gen) {}

    void print(Output &out) const;
    Ref *clone() const { 
      return new Ref(ref,gen);
    }

    bool operator==(const Ref &r) const { 
      return ( (r.ref==ref)&&(r.gen==gen) );
    }
    bool operator!=(const Ref &r) const { 
      return ( (r.ref!=ref)||(r.gen!=gen) );
    }
    bool operator<(const Ref &r) const {
      return (ref<r.ref)||( (ref==r.ref)&&(ref<r.gen) );
    }

    int ref,gen;
  };

} // namespace PDFTools

#endif
