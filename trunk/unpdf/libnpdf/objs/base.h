#ifndef UNPDF_OBJS_BASE_H
#define UNPDF_OBJS_BASE_H

#include <string>
#include <stdarg.h>

namespace PDFTools {

  class Output;
  class Object { // also the null-object
  public:
    virtual ~Object() {}
    virtual void print(Output &out) const;

    virtual Object *clone() const;
    virtual const char *type() const;
  };

  bool isnull(const Object *obj);

} // namespace PDFTools

#endif
