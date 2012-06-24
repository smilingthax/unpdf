#include "aescrypt.h"
#include <assert.h>
#include <string.h>
#include <vector>
#include "exception.h"
#include "../crypt/aescbc.h"
#include "../crypt/rand.h"
#include "../io/base.h"

namespace PDFTools {

StandardAESDecrypt::StandardAESDecrypt(const std::string &key) // {{{
  : key(key)
{
}
// }}}

void StandardAESDecrypt::operator()(std::string &dst,const std::string &src) const // {{{
{
  if ( (src.size()<32)||(src.size()%16) ) {
    throw UsrError("Bad AES crypt length");
  }
  dst.resize(src.size()-16);
  std::vector<char> iv;
  iv.resize(16);
  memcpy(&iv[0],src.data(),16*sizeof(char));
  AESCBC cipher(key,false);
  cipher.decrypt(&dst[0],src.data()+16,dst.size(),&iv[0]);

  // unpad
  const char plen=dst[dst.size()-1];
  if ( (plen<=0)||(plen>16) ) {
    throw UsrError("Bad padding in AES decrypt");
  }
  dst.resize(dst.size()-plen);
}
// }}}

class StandardAESDecrypt::AESInput : public Input {
public:
  AESInput(Input &read_from,const std::string &key);
  int read(char *buf,int len);
  long pos() const;
  void pos(long pos);

protected:
  int read_block(char *buf,int len);
private:
  Input &read_from;
//  std::string key;
  std::vector<char> iv,blkbuf;
  int blkpos,next;
  AESCBC cipher;
};

// {{{ StandardAESDecrypt::AESInput
StandardAESDecrypt::AESInput::AESInput(Input &read_from,const std::string &key) // {{{
  : read_from(read_from),
    cipher(key,false)
{
  blkbuf.resize(16);
  blkpos=blkbuf.size();
  next=-1;
}
// }}}

int StandardAESDecrypt::AESInput::read_block(char *buf,int len) // {{{
{
  // read full blocks
  const int blen=len&~0xf;
  int res;
  if (next>=0) { // 'unread' this character
    buf[0]=next;
    res=read_from.read(buf+1,blen-1)+1;
  } else {
    res=read_from.read(buf,blen);
  }
  if (res%16!=0) {
    throw UsrError("AES stream not multiple of 16");
  }
  cipher.decrypt(buf,buf,res,&iv[0]);
  return res;
}
// }}}

int StandardAESDecrypt::AESInput::read(char *buf,int len) // {{{
{
  int olen=0;
  if (iv.empty()) {
    iv.resize(16);
    int res=read_from.read(&iv[0],16);
    if (res!=16) {
      iv.clear();
      return 0;
    }
  }
  // partial block
  const int blksize=blkbuf.size()-blkpos;
  if (blksize) {
    const int clen=std::min(len,blksize);
    memcpy(buf,&blkbuf[blkpos],blksize*sizeof(char));
    len-=clen;
    buf+=clen;
    olen+=clen;
    blkpos+=clen;
  }
  if (len==0) {
    return olen;
  }
  if (next==-2) { // end-of-stream
    return olen;
  }
  // olen<16
  int res=0;
  if (len>=16) {
    // read full blocks
    res=read_block(buf,len);
    if (res==0) {
      assert(next==-1);
      throw UsrError("AES stream too short");
    }
    buf+=res;
    len-=res;
    olen+=res;
    // res>=16, olen>=16
    next=-1; // maybe we want to read a partial block
  }
  if (len==0) {
    // surely >buf is filled, [olen>=16]
    char c;
    res=read_from.read(&c,1);
    if (res==0) { // no next: unpad
      if ( (buf[-1]<=0)||(buf[-1]>16) ) {
        throw UsrError("Bad padding in AES decrypt");
      }
      olen-=*buf;
      next=-2; // padding seen
    } else {
      next=(unsigned char)c;
    }
  } else {
    // read partial block
    char *bf=&blkbuf[0];
    res=read_block(bf,blkbuf.size());
    if (res==0) { // no next
      if (olen>=16) { // we can unpad
        if ( (buf[-1]<=0)||(buf[-1]>16) ) {
          throw UsrError("Bad padding in AES decrypt");
        }
        olen-=buf[-1];
        next=-2;
      } else {
        assert(next==-1);
        throw UsrError("AES stream too short");
      }
    } else {
      char c;
      res=read_from.read(&c,1);
      if (res==0) { // no next: unpad
        if ( (blkbuf[15]<=0)||(blkbuf[15]>16) ) {
          throw UsrError("Bad padding in AES decrypt");
        }
        blkbuf.resize(16-blkbuf[15]);
        next=-2;
      } else {
        next=(unsigned char)c;
      }

      if (len>(int)blkbuf.size()) {
        len=blkbuf.size();
      }
      memcpy(buf,&blkbuf[0],len*sizeof(char));
      blkpos=len;
      buf+=len;
      olen+=len;
    }
  }
  return olen;
}
// }}}

long StandardAESDecrypt::AESInput::pos() const // {{{
{
  return -1;
}
// }}}

void StandardAESDecrypt::AESInput::pos(long pos) // {{{
{
  if (pos!=0) {
    throw std::invalid_argument("Repositioning of AESInput is not supported");
  }
  blkbuf.resize(16);
  blkpos=blkbuf.size();
  next=-1;
  iv.clear();
  read_from.pos(0);
}
// }}}
// }}}

Input *StandardAESDecrypt::getInput(Input &read_from) const // {{{
{
  return new AESInput(read_from,key);
}
// }}}


StandardAESEncrypt::StandardAESEncrypt(const std::string &key,const std::string &iv)  // {{{
  : key(key),
    useiv(iv.empty()?RAND::get(16):iv)
{
  if (useiv.size()!=16) {
    throw std::invalid_argument("Bad iv size");
  }
}
// }}}

void StandardAESEncrypt::operator()(std::string &dst,const std::string &src,const std::string &_iv) const // {{{
{
  const int plen=16-src.size()%16;
  const int blen=src.size()&~0xf; // complete blocks

  dst.resize(16+blen+16);

  char iv[16];
  if (_iv.empty()) {
    memcpy(iv,RAND::get(16).data(),16*sizeof(char));
  } else {
    if (_iv.size()!=16) {
      throw std::invalid_argument("Bad iv size");
    }
    memcpy(iv,_iv.data(),16*sizeof(char));
//  memcpy(iv,useiv.data(),16*sizeof(char));
  }
  memcpy(&dst[0],iv,16*sizeof(char));

  AESCBC cipher(key,true);
  cipher.encrypt(&dst[16],src.data(),blen,iv);

  char pad[16];
  memset(pad,plen,16*sizeof(char));
  memcpy(pad,src.data()+blen,(16-plen)*sizeof(char));

  cipher.encrypt(&dst[16+blen],pad,16,iv);
}
// }}}

class PDFTools::StandardAESEncrypt::AESOutput : public Output {
public:
  AESOutput(Output &write_to,const std::string &key,const std::string &iv);
  void write(const char *buf,int len);
  void flush();
private:
  Output &write_to;
  AESCBC cipher;
  std::string initiv;
  std::vector<char> iv,tmp,blkbuf;
  int blkpos;
};

// {{{ StandardAESEncrypt::AESOutput
StandardAESEncrypt::AESOutput::AESOutput(Output &write_to,const std::string &key,const std::string &iv) // {{{
  : write_to(write_to),
    cipher(key,true),
    initiv(iv)
{
  tmp.resize(4096); // block size
  blkbuf.resize(16);
  blkpos=blkbuf.size();
}
// }}}

void StandardAESEncrypt::AESOutput::write(const char *buf,int len) // {{{
{
  if (len<0) {
    len=strlen(buf);
  }
  if (iv.empty()) {
    iv.resize(16);
    memcpy(&iv[0],initiv.data(),16*sizeof(char));
    write_to.write(&iv[0],16);
  }
  const int blen=blkbuf.size()-blkpos;
  if (blen) {
    const int clen=std::min(blen,len);
    memcpy(&blkbuf[blkpos],buf,clen*sizeof(char));
    buf+=clen;
    len-=clen;
    blkpos+=clen;
    if (blkpos==(int)blkbuf.size()) {
      cipher.encrypt(&tmp[0],&blkbuf[0],blkbuf.size(),&iv[0]);
      write_to.write(&tmp[0],blkbuf.size());
    } else {
      return;
    }
  }
  while (len>4096) {
    cipher.encrypt(&tmp[0],buf,tmp.size(),&iv[0]);
    write_to.write(&tmp[0],tmp.size());
    buf+=4096;
    len-=4096;
  }
  const int fblock=len&~0xf;
  cipher.encrypt(&tmp[0],buf,fblock,&iv[0]);
  write_to.write(&tmp[0],fblock);
  buf+=fblock;
  len-=fblock;

  if (len) {
    memcpy(&blkbuf[0],buf,len*sizeof(char));
    blkpos=len;
  }
}
// }}}

void StandardAESEncrypt::AESOutput::flush() // {{{
{
  int blen=blkbuf.size()-blkpos;
  if (blen==0) {
    blkpos=0;
    blen=16;
  }
  memset(&blkbuf[blkpos],blen,blen*sizeof(char));

  cipher.encrypt(&tmp[0],&blkbuf[0],blkbuf.size(),&iv[0]);
  write_to.write(&tmp[0],blkbuf.size());
  
  write_to.flush();
  blkpos=blkbuf.size();
  iv.clear();
}
// }}}
// }}}

Output *StandardAESEncrypt::getOutput(Output &write_to) const // {{{
{
  return new AESOutput(write_to,key,useiv);
}
// }}}

} // namespace PDFTools
