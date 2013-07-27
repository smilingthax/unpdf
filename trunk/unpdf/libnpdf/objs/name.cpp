#include "name.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>
#include "../io/base.h"
#include "../parse/ctype.h"

namespace PDFTools {

Name::Name(const char *name, FreeT ft) // {{{
{
  if (ft!=DUP) {
    assert(name);
    val=name;
    ours=(ft==TAKE);
    return;
  }
  val=strdup(name);
  ours=true;
  if (!val) {
    throw std::bad_alloc();
  }
}
// }}}

PDFTools::Name::~Name() // {{{
{
  if (ours) {
    free((void *)val);
  }
}
// }}}

void Name::print(Output &out) const // {{{
{
  assert(val);
  out.put('/');
  for (int iA=0;val[iA];iA++) {
    if ( (iA=='#')||(Parser::is_delim(val[iA])) ) {
      out.printf("#%02x",val[iA]);
      // TODO:  Test it!
      // const char hex[]="0123456789abcdef";
      // maybe: out.put('#');
      // out.put(hex[(val[iA]>>4)&0xf]);
      // out.put(hex[val[iA]&0xf]);
    } else {
      out.put(val[iA]);
    }
  }
//  out.printf("/%s",val);
}
// }}}

} // namespace PDFTools
