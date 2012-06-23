#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <memory>
//#include <stdarg.h>
//#include <errno.h>
//#include "pdfio.h"
//#include "pdfparse.h"
#include "pdfcrypt.h"
#include "pdfsec.h"
#include "exception.h"

using namespace PDFTools;
using namespace std;

// TODO: const Decrypt

// {{{ PDFTools::Decrypt

// }}}

// {{{ PDFTools::Encrypt

// }}}

// {{{ PDFTools::StandardRC4Crypt
PDFTools::StandardRC4Crypt::StandardRC4Crypt(const string &objkey)
  : objkey(objkey)
{
}

void PDFTools::StandardRC4Crypt::operator()(std::string &dst,const std::string &src) const
{
  dst.resize(src.size());
  RC4 cipher(objkey);
  cipher.crypt(&dst[0],src.data(),src.size());
}

// {{{ PDFTools::StandardRC4Crypt::RC4Input
class PDFTools::StandardRC4Crypt::RC4Input : public Input {
public:
  RC4Input(Input &read_from,const std::string &key);
  int read(char *buf,int len);
  long pos() const;
  void pos(long pos);
private:
  Input &read_from;
  RC4 cipher;
};

PDFTools::StandardRC4Crypt::RC4Input::RC4Input(Input &read_from,const string &key) : read_from(read_from),cipher(key)
{
}

int PDFTools::StandardRC4Crypt::RC4Input::read(char *buf,int len)
{
  int res;

  res=read_from.read(buf,len);
  cipher.crypt(buf,buf,res);
  return res;
}

long PDFTools::StandardRC4Crypt::RC4Input::pos() const
{
  return -1;
}

void PDFTools::StandardRC4Crypt::RC4Input::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Repositioning of RC4Input is not supported");
  }
  read_from.pos(0);
  cipher.restart();
}
// }}}

// {{{ PDFTools::StandardRC4Crypt::RC4Output
class PDFTools::StandardRC4Crypt::RC4Output : public Output {
public:
  RC4Output(Output &write_to,const std::string &key);
  void write(const char *buf,int len);
  void flush();
private:
  Output &write_to;
  RC4 cipher;
  std::vector<char> tmp;
};

PDFTools::StandardRC4Crypt::RC4Output::RC4Output(Output &write_to,const string &key) : write_to(write_to),cipher(key)
{
  tmp.resize(4096); // block size
}

void PDFTools::StandardRC4Crypt::RC4Output::write(const char *buf,int len)
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

void PDFTools::StandardRC4Crypt::RC4Output::flush()
{
  write_to.flush();
  cipher.restart();
}
// }}}

Input *PDFTools::StandardRC4Crypt::getInput(Input &read_from) const
{
  return new RC4Input(read_from,objkey);
}

Output *PDFTools::StandardRC4Crypt::getOutput(Output &write_to) const
{
  return new RC4Output(write_to,objkey);
}
// }}}

// {{{ PDFTools::StandardAESDecrypt
PDFTools::StandardAESDecrypt::StandardAESDecrypt(const string &key)
  : key(key)
{
}

void PDFTools::StandardAESDecrypt::operator()(std::string &dst,const std::string &src) const
{
  if ( (src.size()<32)||(src.size()%16) ) {
    throw UsrError("Bad AES crypt length");
  }
  dst.resize(src.size()-16);
  vector<char> iv;
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

// {{{ PDFTools::StandardAESDecrypt::AESInput
class PDFTools::StandardAESDecrypt::AESInput : public Input {
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

PDFTools::StandardAESDecrypt::AESInput::AESInput(Input &read_from,const string &key) : read_from(read_from),cipher(key,false)
{
  blkbuf.resize(16);
  blkpos=blkbuf.size();
  next=-1;
}

int PDFTools::StandardAESDecrypt::AESInput::read_block(char *buf,int len)
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

int PDFTools::StandardAESDecrypt::AESInput::read(char *buf,int len)
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
    const int clen=min(len,blksize);
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

long PDFTools::StandardAESDecrypt::AESInput::pos() const
{
  return -1;
}

void PDFTools::StandardAESDecrypt::AESInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Repositioning of AESInput is not supported");
  }
  blkbuf.resize(16);
  blkpos=blkbuf.size();
  next=-1;
  iv.clear();
  read_from.pos(0);
}
// }}}

Input *PDFTools::StandardAESDecrypt::getInput(Input &read_from) const
{
  return new AESInput(read_from,key);
}
// }}}

// {{{ PDFTools::StandardAESEncrypt
PDFTools::StandardAESEncrypt::StandardAESEncrypt(const string &key,const std::string &iv) 
  : key(key),
    useiv(iv.empty()?RAND::get(16):iv)
{
  if (useiv.size()!=16) {
    throw invalid_argument("Bad iv size");
  }
}

void PDFTools::StandardAESEncrypt::operator()(std::string &dst,const std::string &src,const std::string &_iv) const
{
  const int plen=16-src.size()%16;
  const int blen=src.size()&~0xf; // complete blocks

  dst.resize(16+blen+16);

  char iv[16];
  if (_iv.empty()) {
    memcpy(iv,RAND::get(16).data(),16*sizeof(char));
  } else {
    if (_iv.size()!=16) {
      throw invalid_argument("Bad iv size");
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

// {{{ PDFTools::StandardAESEncrypt::AESOutput
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

PDFTools::StandardAESEncrypt::AESOutput::AESOutput(Output &write_to,const string &key,const std::string &iv) : write_to(write_to),cipher(key,true),initiv(iv)
{
  tmp.resize(4096); // block size
  blkbuf.resize(16);
  blkpos=blkbuf.size();
}

void PDFTools::StandardAESEncrypt::AESOutput::write(const char *buf,int len)
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
    const int clen=min(blen,len);
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

void PDFTools::StandardAESEncrypt::AESOutput::flush()
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

Output *PDFTools::StandardAESEncrypt::getOutput(Output &write_to) const
{
  return new AESOutput(write_to,key,useiv);
}
// }}}

// {{{ PDFTools::StandardSecurityHandler
static void getCFName(PDF &pdf,Dict &dict,const char *key,PDFTools::StandardSecurityHandler::CryptFilter &ret,int &len) // {{{
{
  ObjectPtr optr=dict.get(pdf,key);
  if (optr.empty()) {
    // default is "Identity"
    return;
  }

  const Name *cfname=dynamic_cast<const Name *>(optr.get());
  if (!cfname) {
    throw UsrError("%s is not a Name",key);
  }

  ObjectPtr cfptr=dict.get(pdf,"CF");
  StandardSecurityHandler::get_cf(pdf,cfptr.get(),cfname->value(),ret);

  if (strcmp(key,"EFF")!=0) {
    ret.event=StandardSecurityHandler::CryptFilter::DocOpen; // override for StrF,StmF, per 1.7 spec
  }
  if (len==-1) {
    len=ret.len;
  } else if ( (ret.len!=-1)&&(len!=ret.len) ) {
    throw UsrError("Differing key lengths for StmF,StrF and EFF are not supported (for now?)");
  }
}
// }}}

// TODO: we want to make dict.cf to consist only of direct references
PDFTools::StandardSecurityHandler::StandardSecurityHandler(PDF &pdf,const String &firstId,Dict *edict) : docid(firstId.value())
{
  dict._move_from(edict);
  // we already know: /Filter /Standard

  algo=dict.getInt(pdf,"V",0);
  if ( (algo<1)||(algo>5) ) {
    throw UsrError("Encryption algorithm %d not supported",algo);
  }

  len=40;
  encryptMeta=true; // TODO: respect this later on
  if ( (algo==2)||(algo==3) ) {
    len=dict.getInt(pdf,"Length",40);
  } else if ( (algo==4)||(algo==5) ) {
    encryptMeta=dict.getBool(pdf,"EncryptMetadata",true);

    len=-1;
    getCFName(pdf,dict,"StmF",modecf[StmF],len);
    getCFName(pdf,dict,"StrF",modecf[StrF],len);
    getCFName(pdf,dict,"EFF",modecf[Eff],len);

    if (len<=0) { // esp. ==0
      if (algo==4) {
        len=128;
      } else if (algo==5) {
        len=256;
      }
      // I'm not sure whether V2+(algo>3), AESV2+(algo!=4)  are really allowed
    }
  }
  if ( (len<40)||(len>256)||((len%8)!=0) ) {
    throw UsrError("Bad encryption length: %d",len);
  }

  rev=dict.getInt(pdf,"R");
  if (rev==2) {
    if (algo!=1) {
      throw UsrError("Bad Security Revision");
    }
    assert(len==40);
  } else if (rev==3) {
    if ( (algo!=2)&&(algo!=3) ) {
      throw UsrError("Bad Security Revision");
    }
    if (len>128) {
      throw UsrError("Bad key length");
    }
  } else if (rev==4) { // AES or RC4 (or None) as CryptFilter
    if (algo!=4) {
      throw UsrError("Bad Security Revision");
    }
    if (len!=128) {
      throw UsrError("Bad key length"); // supp.3  clarifies: algo(V)=4  =>  length==128   (addition to 3.5. pg 116)
    }
  } else if (rev==5) {
    if (algo!=5) {
      throw UsrError("Bad Security Revision");
    }
    if (len!=256) {
      throw UsrError("Bad key length");
    }
  } else {
    throw UsrError("SecurityHandler revision %d not supported",rev);
  }

  ownerpw_hash=dict.getString(pdf,"O");
  userpw_hash=dict.getString(pdf,"U");

  perm=dict.getInt(pdf,"P");

  if (rev==5) {
    ownerpw_key=dict.getString(pdf,"OE");
    userpw_key=dict.getString(pdf,"UE");
    perm_enc=dict.getString(pdf,"Perms");
  }
 
  check_pw(string());
}

void PDFTools::StandardSecurityHandler::get_cf(PDF &pdf,const Object *cfdict,const char *cryptname,CryptFilter &ret) // {{{
{
  if (strcmp(cryptname,"Identity")==0) {
    return;
  }

  const Dict *dcf=dynamic_cast<const Dict *>(cfdict);
  if (!dcf) {
    throw UsrError("CF is not a dictionary");
  }

  ObjectPtr optr=dcf->get(pdf,cryptname);
  if (optr.empty()) {
    throw UsrError("Crypt filter %s not found in /CF",cryptname);
  }

  if (!ret.parse(pdf,optr.get())) {
    throw UsrError("Unsupported crypt filter dictionary for %s",cryptname);
  }
}
// }}}

bool PDFTools::StandardSecurityHandler::check_pw(const std::string &pw) // {{{
{
  userpw.clear();
  ownerpw.clear();
  filekey.clear();

  bool owner=false,user=false;
  if (rev==5) {
    owner=check_ownerpw32a(pw);
    if (!owner) {
      user=check_userpw32a(pw);
    }
  } else {
    owner=check_ownerpw(pw);
    if (!owner) {
      user=check_userpw(pw);
    }
  }
  if (owner) {
    return true;
  } else if (user) {
    fprintf(stderr,"NOTE: only permissions %x\n",perm);
    return true;
  }
  fprintf(stderr,"WARNING: not authenticated\n");
  return false;
}
// }}}

bool PDFTools::StandardSecurityHandler::CryptFilter::parse(PDF &pdf,const Object *obj) // {{{
{
  const Dict *dval=dynamic_cast<const Dict *>(obj);
  if (!dval) {
    throw UsrError("CF is not a dictionary");
  }
  dval->getNames(pdf,"Type","CryptFilter",NULL);  // optional

  try {
    int type=dval->getNames(pdf,"CFM","None","V2","AESV2","AESV3",NULL);
    assert( (type>=0)&&(type<=AESV3) );
    method=(Method)type;
  } catch (...) {
    // TODO: better: return false, if not in list
    return false;
  }
  if (method==None) {
    throw UsrError("/None is not supported"); // TODO: we don't understand this fully
  }
  int authevent=dval->getNames(pdf,"AuthEvent","DocOpen","EFOpen",NULL); // only DocOpen, cf. 1.7 Supp.3  (EFOpen deprecated?) -- 1.7: if used as StmCF,StrCF: ignore and always use DocOpen
  event=(AuthEvent)authevent;

  len=dval->getInt(pdf,"Length",0)*8; // see errata!

  return true;
}
// }}}

const char PDFTools::StandardSecurityHandler::pad[]=
         "\x28\xbf\x4e\x5e\x4e\x75\x8a\x41\x64\x00\x4e\x56\xff\xfa\x01\x08"
         "\x2e\x2e\x00\xb6\xd0\x68\x3e\x80\x2f\x0c\xa9\xfe\x64\x53\x69\x7a";

// compute crypt-key from user_pw 
// NOTE: user_pw shall be converted from user codepage (UTF-8 via user cp) to PDFDocEncoding
string PDFTools::StandardSecurityHandler::compute_key(const string &user_pw) // {{{
{
  const int plen=(user_pw.size()>32)?32:user_pw.size();
  char buf[16];

  MD5 hash;
  hash.init();
  hash.update(user_pw.data(),plen);
  hash.update(pad,32-plen);
  hash.update(ownerpw_hash);
  buf[0]=(perm)&0xff;
  buf[1]=(perm>>8)&0xff;
  buf[2]=(perm>>16)&0xff;
  buf[3]=(perm>>24)&0xff;
  hash.update(buf,4);
  hash.update(docid);
  if ( (rev==4)&&(!encryptMeta) ) {
    buf[0]=buf[1]=buf[2]=buf[3]=0xff;
    hash.update(buf,4);
  }
  hash.finish(buf);
  if (rev>=3) {
    for (int iA=0;iA<50;iA++) {
      MD5::md5(buf,buf,len/8);
    }
  }
  return string(buf,len/8);
}
// }}}

string PDFTools::StandardSecurityHandler::compute_user_hash(const string &ukey) // {{{
{
  if (rev==2) {
    RC4 cipher(ukey);

    string ret(32,0);
    cipher.crypt(&ret[0],pad,32);
    return ret;
  } else if (rev>=3) {
    char key[16];
    assert(ukey.size()<=16);
    memcpy(key,ukey.data(),ukey.size());

    MD5 hash;
    char buf[16];

    hash.init();
    hash.update(pad,32);
    hash.update(docid);
    hash.finish(buf);

    RC4 cipher;
    cipher.setkey(key,ukey.size());
    cipher.crypt(buf,buf,16);
    for (int iA=1;iA<20;iA++) {
      for (int iB=0;iB<(int)ukey.size();iB++) {
        key[iB]^=(iA-1)^iA;
      }
      cipher.setkey(key,ukey.size());
      cipher.crypt(buf,buf,16);
    }
    return string(buf,16);
  }
  throw UsrError("Bad revision");
}
// }}}

string PDFTools::StandardSecurityHandler::compute_owner_key(const string &owner_pw) // {{{
{
  const int plen=(owner_pw.size()>32)?32:owner_pw.size();
  char key[16];

  MD5 hash;
  hash.init();
  hash.update(owner_pw.data(),plen);
  hash.update(pad,32-plen);
  hash.finish(key);
  if (rev>=3) {
    for (int iA=0;iA<50;iA++) {
      MD5::md5(key,key,16);
    }
  }
  return string(key,len/8);
}
// }}}

// the owner-hash is the plain user_pw crypted by the owner_key
string PDFTools::StandardSecurityHandler::encrypt_owner_hash(const string &owner_key,const string &user_pw) // {{{
{
  const int qlen=(user_pw.size()>32)?32:user_pw.size();
  char key[16];
  assert(owner_key.size()<=16);
  memcpy(key,owner_key.data(),owner_key.size());

  RC4 cipher;
  cipher.setkey(key,owner_key.size());

  char buf[32];
  memcpy(buf,user_pw.data(),qlen*sizeof(char));
  memcpy(buf+qlen,pad,(32-qlen)*sizeof(char));

  cipher.crypt(buf,buf,32);
  if (rev>=3) {
    for (int iA=1;iA<20;iA++) {
      for (int iB=0;iB<(int)owner_key.size();iB++) {
        key[iB]^=(iA-1)^iA;
      }
      cipher.setkey(key,len/8);
      cipher.crypt(buf,buf,32);
    }
  }
  return string(buf,32);
}
// }}}

// returns alleged(,padded) user_pw
string PDFTools::StandardSecurityHandler::decrypt_owner_hash(const string &owner_key,const string &owner_hash) // {{{
{
  char buf[32];
  assert(owner_hash.size()==32);
  memcpy(buf,owner_hash.data(),32);

  if (rev==2) {
    RC4 cipher(owner_key);

    cipher.crypt(buf,buf,32);
    return string(buf,32);
  } else if (rev>=3) {
    char key[16];
    assert(owner_key.size()<=16);

    RC4 cipher;
    for (int iA=19;iA>=0;iA--) {
      for (int iB=0;iB<(int)owner_key.size();iB++) {
        key[iB]=owner_key[iB]^iA;
      }
      cipher.setkey(key,owner_key.size());
      cipher.crypt(buf,buf,32);
    }
    return string(buf,32);
  }
  throw UsrError("Bad revision");
}
// }}}

bool PDFTools::StandardSecurityHandler::check_userpw(const string &user_pw) // {{{
{
  string key=compute_key(user_pw);
  string u_hash=compute_user_hash(key);

  if (userpw_hash.size()!=32) {
    throw UsrError("Encryption /U entry not 32 bytes long");
  }
  if (memcmp(u_hash.data(),userpw_hash.data(),u_hash.size())==0) {
    userpw=user_pw;
    filekey=key;
    return true;
  }
  return false;
}
// }}}

bool PDFTools::StandardSecurityHandler::check_ownerpw(const string &owner_pw) // {{{
{
  if (ownerpw_hash.size()!=32) {
    throw UsrError("Encryption /O entry not 32 bytes long");
  }

  string owner_key=compute_owner_key(owner_pw);
  string user_pw=decrypt_owner_hash(owner_key,ownerpw_hash);

  if (check_userpw(user_pw)) { // will set filekey
    ownerpw=owner_pw;
    return true;
  }
  return false;
}
// }}}

void PDFTools::StandardSecurityHandler::validate_perm() // {{{
{
  assert(filekey.size()==32);
  if (perm_enc.size()!=16) {
    throw UsrError("Encryption /Perms entry not 32 bytes long");
  }

  // reference says ECB, but ECB has no IV! (still, because of no_blocks=1, this is equal to CBC with IV=0).
  AESCBC aes(filekey,false);
  char iv[16]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0},buf[16];

  aes.decrypt(buf,perm_enc.data(),16,iv);

  if ( (buf[9]!='a')||(buf[10]!='d')||(buf[11]!='b') ) {
    fprintf(stderr,"Warning: Invalid Perms identifier\n");
    return;
  }
  if ( (buf[8]!='T')&&(buf[8]!='F') ) {
    fprintf(stderr,"Warning: Bad /Perms\n");
    return;
  }

  int p=((unsigned char)buf[0])|((unsigned char)buf[1]<<8)|((unsigned char)buf[2]<<16)|((unsigned char)buf[3]<<24);
  if (p!=perm) {
    fprintf(stderr,"Warning: Perms mismatch\n");
    // TODO?  perm=p;
  }
  if ((buf[8]=='T')!=encryptMeta) {
    fprintf(stderr,"Warning: EncryptMeta mismatch\n");
    // TODO?  encryptMeta=(buf[8]=='T');
  }
}
// }}}

std::string saslprep(const std::string &in)
{
/*
  // TODO: SASLprep (RFC 4013 with BIDI, Normalize)
*/
  return in;
}

// NOTE: pw in UTF-8
bool PDFTools::StandardSecurityHandler::check_ownerpw32a(const string &pw) // {{{
{
  if (ownerpw_hash.size()!=48) {
    throw UsrError("Encryption /O entry not 48 bytes long (rev 5)");
  }
  if (ownerpw_key.size()!=32) {
    throw UsrError("Encryption /OE entry not 32 bytes long");
  }
  
  string owner_pw=saslprep(pw);
  const int plen=(owner_pw.size()>127)?127:owner_pw.size();
  char buf[32];
  SHA256 hash;

  hash.init();
  hash.update(owner_pw.data(),plen);
  hash.update(ownerpw_hash.data()+32,8);
  hash.update(userpw_hash);
  hash.finish(buf);

  if (memcmp(buf,ownerpw_hash.data(),32)==0) {
    // owner pw correct!
    hash.init();
    hash.update(owner_pw.data(),plen);
    hash.update(ownerpw_hash.data()+40,8);
    hash.update(userpw_hash);
    hash.finish(buf);
    ownerpw=pw; // owner_pw?

    AESCBC aes(buf,false);
    char iv[16]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
    filekey.resize(32);
    aes.decrypt(&filekey[0],ownerpw_key.data(),32,iv);

    validate_perm();
    return true;
  }
  return false;
}
// }}}

bool PDFTools::StandardSecurityHandler::check_userpw32a(const string &pw) // {{{
{
  if (userpw_hash.size()!=48) {
    throw UsrError("Encryption /U entry not 48 bytes long (rev 5)");
  }
  if (userpw_key.size()!=32) {
    throw UsrError("Encryption /OE entry not 32 bytes long");
  }

  string user_pw=saslprep(pw);
  const int plen=(user_pw.size()>127)?127:user_pw.size();
  char buf[32];
  SHA256 hash;

  hash.init();
  hash.update(user_pw.data(),plen);
  hash.update(userpw_hash.data()+32,8);
  hash.finish(buf);

  if (memcmp(buf,userpw_hash.data(),32)==0) {
    // user pw correct!
    hash.init();
    hash.update(user_pw.data(),plen);
    hash.update(ownerpw_hash.data()+40,8);
    hash.finish(buf);
    userpw=pw; // user_pw?

    AESCBC aes(buf,false);
    char iv[16]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
    filekey.resize(32);
    aes.decrypt(&filekey[0],userpw_key.data(),32,iv);

    validate_perm();
    return true;
  }
  return false;
}
// }}}


pair<string,string> PDFTools::StandardSecurityHandler::set_pw(const string &owner_pw,const string &user_pw)
{
  string key=compute_key(user_pw);
  string u_hash=compute_user_hash(key);
  if (rev>=3) {
    u_hash.resize(32,0);
  }
  assert(u_hash.size()==32);

  string owner_key=compute_owner_key(owner_pw);
  string o_hash=encrypt_owner_hash(owner_key,user_pw);

  return make_pair(o_hash,u_hash);
}

pair<string,string> PDFTools::StandardSecurityHandler::set_pw(const string &user_pw)
{
  return set_pw(user_pw,user_pw);
}

pair<string,string> PDFTools::StandardSecurityHandler::set_userpw38(const string &key,const string &pw) // {{{
{
  assert(key.size()==32);
  string salt=RAND::get(16);

  string user_pw=saslprep(pw);
  const int plen=(user_pw.size()>127)?127:user_pw.size();
  char buf[32];
  SHA256 hash;

  hash.init();
  hash.update(user_pw.data(),plen);
  hash.update(salt.data(),8);
  hash.finish(buf);
  string u_hash(buf,32);
  u_hash.append(salt);

  hash.init();
  hash.update(user_pw.data(),plen);
  hash.update(salt.data()+8,8);
  hash.finish(buf);

  AESCBC aes(buf,true);
  char iv[16]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
  aes.encrypt(buf,key.data(),32,iv);

  assert(u_hash.size()==48);
  return make_pair(u_hash,string(buf,32));
}
// }}}

pair<string,string> PDFTools::StandardSecurityHandler::set_ownerpw39(const string &key,const string &u_hash,const string &pw) // {{{
{
  assert(key.size()==32);
  assert(u_hash.size()==48);
  string salt=RAND::get(16);

  string owner_pw=saslprep(pw);
  const int plen=(owner_pw.size()>127)?127:owner_pw.size();
  char buf[32];
  SHA256 hash;

  hash.init();
  hash.update(owner_pw.data(),plen);
  hash.update(salt.data(),8);
  hash.update(u_hash);
  hash.finish(buf);
  string o_hash(buf,32);
  o_hash.append(salt);

  hash.init();
  hash.update(owner_pw.data(),plen);
  hash.update(salt.data()+8,8);
  hash.update(u_hash);
  hash.finish(buf);

  AESCBC aes(buf,true);
  char iv[16]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
  aes.encrypt(buf,key.data(),32,iv);

  assert(o_hash.size()==48);
  return make_pair(o_hash,string(buf,32));
}
// }}}

Dict *PDFTools::StandardSecurityHandler::set_pw389(const string &owner_pw,const string &user_pw,int perms,bool encmeta) // {{{
{
  string key=RAND::get(32);

  pair<string,string> uue=set_userpw38(key,user_pw);
  pair<string,string> ooe=set_ownerpw39(key,uue.first,owner_pw);

  char buf[16],iv[16]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

  memset(buf,0xff,8);
  buf[0]=((unsigned int)perms)&0xff;
  buf[1]=((unsigned int)perms>>8)&0xff;
  buf[2]=((unsigned int)perms>>16)&0xff;
  buf[3]=((unsigned int)perms>>24)&0xff;

  if (encmeta) {
    buf[8]='T'; 
  } else {
    buf[8]='F'; 
  }

  buf[9]='a';
  buf[10]='d';
  buf[11]='b';

  memcpy(buf+12,RAND::get(4).data(),4);

  AESCBC aes(key,true);
  aes.encrypt(buf,buf,16,iv);

  auto_ptr<Dict> ret(new Dict);
  ret->add("Filter","Standard",Name::STATIC);
//  ret->add("SubFilter",...);
  ret->add("V",5); // TODO?  >=5 required, >5 not yet defined
/* Still "missing", to be added by caller
  ret->add("CF");
  ret->add("StmF");
  ret->add("StrF");
  ret->add("EFF");
*/

  ret->add("R",5); // see /V
  ret->add("U",new String(uue.first),true);
  ret->add("UE",new String(uue.second),true);
  ret->add("O",new String(ooe.first),true);
  ret->add("OE",new String(ooe.second),true);
  ret->add("P",perms);
  ret->add("Perms",new String(buf,16),true);
  ret->add("EncryptMetadata",encmeta);

  return ret.release();
}
// }}}

string PDFTools::StandardSecurityHandler::get_objkey(const Ref &ref,bool aes) // {{{ 
{
  assert(algo<5);
  assert( (!aes)||(algo==4) );  // note that (algo==4)&&(!aes) is permissible

  MD5 hash;
  char buf[5];

  hash.init();
  hash.update(filekey);

  if (algo==3) { // "unpublished algorithm"
    buf[0]=((ref.ref)&0xff)^0xac;
    buf[1]=((ref.gen)&0xff)^0x96;
    buf[2]=((ref.ref>>8)&0xff)^0x69;
    buf[3]=((ref.gen>>8)&0xff)^0xca;
    buf[4]=((ref.ref>>16)&0xff)^0x35;
  } else {
    buf[0]=(ref.ref)&0xff;
    buf[1]=(ref.ref>>8)&0xff;
    buf[2]=(ref.ref>>16)&0xff;
    buf[3]=(ref.gen)&0xff;
    buf[4]=(ref.gen>>8)&0xff;
  }
  hash.update(buf,5);
  if ( (algo==3)||(aes) ) { 
    hash.update("sAlT",4);
  }

  string ret(16,0);
  hash.finish(&ret[0]);

  if ((filekey.size()+5)<16) {
    assert(algo!=4);
    ret.resize(filekey.size()+5);
  }
  return ret;
}
// }}}

// TODO: allow non-/Identity Crypt filters
// pdf17_errata prescribes a few CF names for acro compatibility!
Decrypt *PDFTools::StandardSecurityHandler::getDecrypt(const Ref &ref,CFMode cfm,const char *cryptname) // {{{
{
  if ( (cfm<0)||(cfm>Eff) ) {
    throw invalid_argument("Bad CFMode");
  }
  if (filekey.empty()) { // not authenticated
    return NULL;
  }
  CryptFilter tmpcf,*cf=NULL;
  if (cryptname) {
    /* TODO:   (we don't have pdf around!)   
               ... we could get to it
               ... parseObj actually takes care that we /could/ seek  (for strings)
               ... IFilter has also pdf around
    ObjectPtr cfptr=dict.get(pdf,"CF");
    get_cf(pdf,cfptr.get(),cryptname,tmpcf);
    cf=&tmpcf;
    */ 
    if (strcmp(cryptname,"Identity")==0) { // only recognized case for now
      return NULL;
    } else {
      fprintf(stderr,"WARNING: Non-default crypt filter not supported\n");
    }
  } else if (algo>=4) { // using default(!) crypt filter
    cf=&modecf[cfm];
  }
  if (cf) {
    if (cf->len==-1) { // identity
      return NULL;
    } else if (cf->method==CryptFilter::V2) {
      return new StandardRC4Crypt(get_objkey(ref,false));
    } else if (cf->method==CryptFilter::AESV2) {
      return new StandardAESDecrypt(get_objkey(ref,true));
    } else if (cf->method==CryptFilter::AESV3) {
      assert(filekey.size()==256);
      return new StandardAESDecrypt(filekey);
    } else {
      fprintf(stderr,"WARNING: /None decryption untested\n");
      return NULL;
    }
  }
  return new StandardRC4Crypt(get_objkey(ref,false));
}
// }}}

// }}}

