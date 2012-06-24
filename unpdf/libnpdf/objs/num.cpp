#include "num.h"
#include <stdio.h>
#include <math.h>
#include "../io/base.h"

namespace PDFTools {

#define EPS 1e-8
void fminout(Output &out,float val) // {{{
{
  // output only required number of digits
  float ival=truncf(val),fval=fabsf(val-ival);
  out.printf("%.0f",ival);
  if (fval>EPS) {
    out.puts(".");
    float tmp=fval;
    while (tmp>EPS) {
      fval*=10;
      out.printf("%d",(int)fmodf(fval,10));
      tmp/=10;
    }
  }
}
// }}}


// {{{ Boolean
Boolean::Boolean(bool value) : val(value)
{
}

void Boolean::print(Output &out) const
{
  if (val) {
    out.puts("true");
  } else {
    out.puts("false");
  }
}
// }}}

// {{{ NumInteger
NumInteger::NumInteger(int value) : val(value)
{
}

void NumInteger::print(Output &out) const
{
  out.printf("%d",val);
}
// }}}

// {{{ NumFloat
NumFloat::NumFloat(float value) : val(value)
{
}

void NumFloat::print(Output &out) const
{
  fminout(out,val);
//  out.printf("%f",val);
}
// }}}

} // namespace PDFTools
