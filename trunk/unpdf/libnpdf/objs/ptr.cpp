#include "ptr.h"
#include <assert.h>

namespace PDFTools {

ObjectPtr::ObjectPtr(ObjectPtr &optr) // {{{
  : ptr(optr.release()),
    ours(optr.ours)
{
}
// }}}

ObjectPtr &ObjectPtr::operator=(ObjectPtr &optr) // {{{
{
  reset(optr.release(),optr.ours);
  return *this;
}
// }}}

ObjectPtr &ObjectPtr::operator=(ObjectPtr_ref ref) // {{{
{
  reset(ref.ptr,ref.ours);
  return *this;
}
// }}}

void ObjectPtr::reset(Object *obj,bool take) // {{{
{
  if (obj!=ptr) {
    if (ours) {
      delete ptr;
    }
    ours=take;
    ptr=obj;
  } else {
    assert(ours==take);
  }
}
// }}}

} // namespace PDFTools
