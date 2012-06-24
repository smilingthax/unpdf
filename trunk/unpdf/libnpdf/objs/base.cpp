#include "base.h"
#include <stdio.h>
#include <typeinfo>
#include "exception.h"
#include "../io/base.h"

namespace PDFTools {

// {{{ Object
void Object::print(Output &out) const 
{
  out.puts("null");
}

Object *Object::clone() const
{
  if (!isnull(this)) {
    throw UsrError("clone() not implemented for %s",typeid(*this).name());
  }
  return new Object();
}

const char *Object::type() const
{
  return typeid(*this).name();
}
// }}}

bool isnull(const Object *obj) // {{{
{
  if (obj) {
    return (typeid(*obj)==typeid(const Object));
  }
  fprintf(stderr,"WARNING: isnull(NULL)\n");
  return false;
}
// }}}

} // namespace PDFTools
