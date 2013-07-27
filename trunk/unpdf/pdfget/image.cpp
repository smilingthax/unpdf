#include "image.h"
#include <stdexcept>

namespace PDFTools {

bool validBPC(int bpc) // {{{
{
  return (bpc==1)||(bpc==2)||(bpc=4)||(bpc==8)||
         (/*pdf16*/bpc==16);
}
// }}}


#define BLOCK_SIZE 4096
DecodeFilter::FInput::FInput(Input &read_from,int color,int bpc,const std::vector<float> *decode,bool invert) // {{{
  : read_from(read_from),
    color(color),bpc(bpc),
    decode(decode),invert(invert)
{
  reset();
}
// }}}

static float interp(int val,int maxval,float dmin,float dmax) // {{{
{
  float ret=val*(dmax-dmin)/maxval;
  if (ret<0) {
    return dmin;
  }
  ret+=dmin;
  if (ret>dmax) {
    return dmax;
  }
  return ret;
}
// }}}

// TODO: bool ColorSpace::is_default_decode(...)

// TODO: FIXME: do not read   char*   but  float*
int DecodeFilter::FInput::read(char *buf,int len) // {{{
{
  int olen=0;
#if 0

  if (len==0) {
    return 0;
  }
  while (1) {
    for (;inpos<(int)block.size();inpos++) {
      ...

TODO: process in chunks of bpc*color
  const int maxval=(1<<bpc)-1;
  for (int col=0; col<color; col++) {
    int val=get(block[...]);

    if (decode) {
      float fval=interp(val,maxval,(*decode)[2*col],(*decode)[2*col+1]);
    } // TODO?! else default: val/maxval    or indentity: val
  }

      if (len==0) {
        return olen;
      }
...
    }
    block.resize(BLOCK_SIZE);
    int res=read_from.read(&block[0],block.size());
    if (res==0) {
      throw UsrError("Premature end");
    }
    block.resize(res);
    inpos=0;
  }
#endif
}
// }}}

long DecodeFilter::FInput::pos() const // {{{
{
  return -1;
}
// }}}

void DecodeFilter::FInput::pos(long pos) // {{{
{
  if (pos!=0) {
    throw std::invalid_argument("Reposition in DecodeFilter_Input is not supported");
  }
  reset();
  read_from.pos(0);
}
// }}}

void DecodeFilter::FInput::reset() // {{{
{
  block.resize(BLOCK_SIZE);
  inpos=block.size();
}
// }}}
#undef BLOCK_SIZE

/*
DecodeFilter::FInput::

Input &read_from;
int color,bpc;
const std::vector<float> *decode;
bool invert;

std::vector<char> block;
int inpos;
*/

} // namespace PDFTools
