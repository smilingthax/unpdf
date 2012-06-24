#include "rc4crypt.h"
#include <stdexcept>
#include <vector>
#include <string.h>
#include "../crypt/rc4.h"
#include "../io/base.h"

namespace PDFTools {

StandardRC4Crypt::StandardRC4Crypt(const std::string &objkey) // {{{
  : objkey(objkey)
{
}
// }}}

void StandardRC4Crypt::operator()(std::string &dst,const std::string &src) const // {{{
{
  dst.resize(src.size());
  RC4 cipher(objkey);
  cipher.crypt(&dst[0],src.data(),src.size());
}
// }}}

class StandardRC4Crypt::RC4Input : public Input {
public:
  RC4Input(Input &read_from,const std::string &key);
  int read(char *buf,int len);
  long pos() const;
  void pos(long pos);
private:
  Input &read_from;
  RC4 cipher;
};

// {{{ StandardRC4Crypt::RC4Input
StandardRC4Crypt::RC4Input::RC4Input(Input &read_from,const std::string &key)
  : read_from(read_from),
    cipher(key)
{
}

int StandardRC4Crypt::RC4Input::read(char *buf,int len)
{
  int res=read_from.read(buf,len);
  cipher.crypt(buf,buf,res);
  return res;
}

long StandardRC4Crypt::RC4Input::pos() const
{
  return -1;
}

void StandardRC4Crypt::RC4Input::pos(long pos)
{
  if (pos!=0) {
    throw std::invalid_argument("Repositioning of RC4Input is not supported");
  }
  read_from.pos(0);
  cipher.restart();
}
// }}}

class StandardRC4Crypt::RC4Output : public Output {
public:
  RC4Output(Output &write_to,const std::string &key);
  void write(const char *buf,int len);
  void flush();
private:
  Output &write_to;
  RC4 cipher;
  std::vector<char> tmp;
};

// {{{ StandardRC4Crypt::RC4Output
StandardRC4Crypt::RC4Output::RC4Output(Output &write_to,const std::string &key)
  : write_to(write_to),
    cipher(key)
{
  tmp.resize(4096); // block size
}

void StandardRC4Crypt::RC4Output::write(const char *buf,int len)
{
  if (len<0) {
    len=strlen(buf);
  }
  while (len>4096) {
    cipher.crypt(&tmp[0],buf,tmp.size());
    write_to.write(&tmp[0],tmp.size());
    buf+=4096;
    len-=4096;
  }
  cipher.crypt(&tmp[0],buf,len);
  write_to.write(&tmp[0],len);
}

void StandardRC4Crypt::RC4Output::flush()
{
  write_to.flush();
  cipher.restart();
}
// }}}

Input *StandardRC4Crypt::getInput(Input &read_from) const
{
  return new RC4Input(read_from,objkey);
}

Output *StandardRC4Crypt::getOutput(Output &write_to) const
{
  return new RC4Output(write_to,objkey);
}

} // namespace PDFTools
