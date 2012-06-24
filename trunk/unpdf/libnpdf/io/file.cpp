#include "file.h"
#include <assert.h>
#include <errno.h>
#include "exception.h"
#include "filesystem.h"
#include "tools.h"

namespace PDFTools {

// {{{ FILEInput
FILEInput::FILEInput(FILE *f,bool take) 
  : f(f),
    ourclose(take)
{
  if (!f) {
    throw std::invalid_argument("NULL pointer");
  }
}

FILEInput::FILEInput(const char *filename)
{
  if (!filename) {
    throw std::invalid_argument("NULL pointer");
  }
  if ((f=fopen(filename,"rb"))==NULL) {
    throw FS_except(errno);
  }
  ourclose=true;
}

FILEInput::~FILEInput()
{
  if (ourclose) {
    fclose(f);
  }
}

int FILEInput::read(char *buf,int len)
{
  return fread(buf,1,len,f);
}

long FILEInput::pos() const
{
  return ftell(f);
}

void FILEInput::pos(long pos)
{
  if (fseek(f,pos,(pos<0)?SEEK_END:SEEK_SET)!=0) {
    throw FS_except(errno);
  }
}
// }}}

// {{{ FILEOutput
FILEOutput::FILEOutput(FILE *f,bool take)
  : f(f),
    ourclose(take),
    sumout(0)
{
  if (!f) {
    throw std::invalid_argument("NULL pointer");
  }
}

FILEOutput::FILEOutput(const char *filename) : sumout(0)
{
  if (!filename) {
    throw std::invalid_argument("NULL pointer");
  }
  if ((f=fopen(filename,"wb"))==NULL) {
    throw FS_except(errno);
  }
  ourclose=true;
}

// simplify  output to file, or stdout if ==NULL  case
FILEOutput::FILEOutput(const char *filename,FILE *_f) : sumout(0)
{
  if (filename) { // takes precedence
    if ((f=fopen(filename,"w"))==NULL) {
      throw FS_except(errno);
    }
    ourclose=true;
  } else if (_f) {
    f=_f;
    ourclose=false;
  } else { // !filename && !_f
    throw std::invalid_argument("NULL pointer");
  }
}

FILEOutput::~FILEOutput()
{
  if (ourclose) {
    fclose(f);
  }
}

void FILEOutput::vprintf(const char *fmt,va_list ap)
{
  int ret=vfprintf(f,fmt,ap);
  if (ret<0) {
    throw FS_except(errno); // ? 
  }
  sumout+=ret;
}

void FILEOutput::write(const char *buf,int len)
{
  if (len<0) {
    len=strlen(buf);
  }
  int ret=fwrite(buf,1,len,f);
  if (ret<len) {
    if (errno!=0) {
      throw FS_except(errno);
    } else {
      throw UsrError("Short write");
    }
  }
  sumout+=ret;
}

long FILEOutput::sum() const
{
  long ret=ftell(f);
  if (ret==-1) { // fallback
    return sumout; 
  }
  return ret;
}

void FILEOutput::flush()
{
  fflush(f);
}
// }}}

} // namespace PDFTools
