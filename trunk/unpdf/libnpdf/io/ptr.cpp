#include "ptr.h"
#include <assert.h>
#include <stdio.h>
#include "tools.h"

namespace PDFTools {

void InputPtr::reset(Input *newin,bool take) // {{{
{
  assert( (!in)||(newin!=in) );
  if (closefn) {
    closefn(in,user);
    closefn=NULL;
    user=NULL;
  }
  if (ours) {
    delete in;
  }
  in=newin;
  ours=take;
}
// }}}

void OutputPtr::reset(Output *newout,bool take) // {{{
{
  assert( (!out)||(newout!=out) );
  if (closefn) {
    closefn(out,user);
    closefn=NULL;
    user=NULL;
  }
  if (ours) {
    delete out;
  }
  out=newout;
  ours=take;
}
// }}}

} // namespace PDFTools
