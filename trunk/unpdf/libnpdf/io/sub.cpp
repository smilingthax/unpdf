#include "sub.h"
#include <assert.h>
#include <errno.h>
#include "exception.h"
#include "filesystem.h"

namespace PDFTools {

// {{{ SubInput
SubInput::SubInput(Input &_read_from,long _startpos,long _endpos)
    : parent(dynamic_cast<SubInput *>(&_read_from)),
      read_from((parent)?parent->read_from:_read_from),
      startpos(_startpos),
      endpos(_endpos)
{
  if (parent) { // special case: SubInput of SubInput
    startpos+=parent->startpos;
    if (endpos!=-1) {
      endpos+=parent->startpos;
    }
    if ( (parent->endpos!=-1)&&(parent->endpos<endpos) ) {
      startpos=-1;
    }
  }
  pos(0);
}

bool SubInput::empty() const 
{
  return (startpos==-1);
}

long SubInput::basepos() const
{
  assert(!empty());
  return startpos+cpos;
}

int SubInput::read(char *buf,int len)
{
  if (startpos==-1) {
    return 0;
  }
  if (endpos>=0) {
    const int remain=(endpos-startpos)-cpos;
    if (remain<len) {
      len=remain;
    }
  }
  int res=read_from.read(buf,len);
  cpos+=res;
  return res;
}

long SubInput::pos() const
{
  return cpos;
}

void SubInput::pos(long pos)
{
  if (pos<0) {
    if (endpos<0) {
      assert(0);
      throw UsrError("Not supported"); // TODO?
    }
    pos+=endpos;
  } else if (startpos>=0) {
    pos+=startpos;
  }
  if (  (pos<startpos)||( (endpos>=0)&&(pos>endpos) )  ) {
    throw FS_except(EINVAL);
  }
  read_from.pos(pos);
  cpos=pos-startpos;
}
// }}}

} // namespace PDFTools
