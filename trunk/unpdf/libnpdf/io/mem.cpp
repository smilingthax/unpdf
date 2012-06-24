#include "mem.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "filesystem.h"

namespace PDFTools {

// {{{ MemInput
MemInput::MemInput(const char *buf,int len)
  : buf(buf),
    len(len),
    cpos(0)
{
  if (!buf) {
    throw std::invalid_argument("NULL pointer");
  }
}

int MemInput::read(char *_buf,int _len)
{
  const int clen=std::min(_len,len-(int)cpos);
  memcpy(_buf,buf+cpos,clen*sizeof(char));
  cpos+=clen;
  return clen;
}

long MemInput::pos() const
{
  return cpos;
}

void MemInput::pos(long pos)
{
  if (pos<0) {
    pos+=len;
  }
  if ( (pos<0)||(pos>len) ) {
    throw FS_except(EINVAL);
  }
  cpos=pos;
}

int MemInput::size() const
{
  return len;
}
// }}}

// {{{ MemIOput
MemIOput::MemIOput()
  : cpos(0)
{
}

int MemIOput::read(char *_buf,int len)
{
  const int clen=std::min((int)buf.size()-(int)cpos,len);
  memcpy(_buf,&buf[cpos],clen*sizeof(char));
  cpos+=clen;
  return clen;
}

long MemIOput::pos() const
{
  return cpos;
}

void MemIOput::pos(long pos)
{
  if (pos<0) {
    pos+=buf.size();
  }
  if ( (pos<0)||(pos>(int)buf.size()) ) {
    throw FS_except(EINVAL);
  }
  cpos=pos;
}

void MemIOput::write(const char *_buf,int len)
{
  if (len<0) {
    len=strlen(_buf);
  }
  buf.insert(buf.end(),_buf,_buf+len);
}

int MemIOput::size() const
{
  return buf.size();
}
// }}}

} // namespace PDFTools
