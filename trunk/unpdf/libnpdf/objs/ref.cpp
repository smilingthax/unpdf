#include "ref.h"
#include "../io/base.h"

namespace PDFTools {

void Ref::print(Output &out) const // {{{
{
  out.printf("%d %d R",ref,gen);
}
// }}}

} // namespace PDFTools
