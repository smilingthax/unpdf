#include "ptr.h"
#include <assert.h>

namespace PDFTools {

void TPtr_base::assert_same(bool oldTake,bool newTake)
{
  assert(oldTake==newTake);
}

/*
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
*/

} // namespace PDFTools
