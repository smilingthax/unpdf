#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "pdfio.h"
#include "pdfparse.h"
#include "pdfcrypt.h"
#include "exception.h"
#include "tools.h"

using namespace PDFTools;
using namespace std;

// {{{ PDFTools::Input
string PDFTools::Input::gets(int len) 
{
  int rlen;
#define BUFLEN 512
  char buf[BUFLEN];
  string ret;

  if (len<0) { // read to newline
    // TODO: optimize
    char c;
    for (;len>0;len--) {
      rlen=read(&c,1);
      if (rlen!=1) {
        return ret;
      }
      ret.push_back(c);
    }
    return ret;
  }
#if 0
  do {
    const int l=min(len,BUFLEN);
    rlen=read(buf,l);
    if (rlen<0) {
      return ret;
    }
    len-=rlen;
    ret.append(buf,rlen);
  } while ( (len==0)||(rlen<l) );
#else
  ret.resize(len);
  rlen=read(&ret[0],len);
  ret.resize(rlen);
#endif
  return ret;
}
// }}}

// {{{ PDFTools::Output
void PDFTools::Output::printf(const char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  vprintf(fmt,ap);
  va_end(ap);
}

void PDFTools::Output::vprintf(const char *fmt,va_list ap)
{
  puts(s_vsprintf(fmt,ap).c_str());
}

void PDFTools::Output::puts(const char *str)
{
  write(str,-1);
}

void PDFTools::Output::put(const char c)
{
  write(&c,1);
}

void PDFTools::Output::flush()
{
}
// }}}

// {{{ PDFTools::InputPtr
void PDFTools::InputPtr::reset(Input *newin,bool take)
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

// {{{ PDFTools::OutputPtr
void PDFTools::OutputPtr::reset(Output *newout,bool take)
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

// {{{ PDFTools::FILEInput
PDFTools::FILEInput::FILEInput(FILE *f) : f(f),ourclose(false)
{
  if (!f) {
    throw invalid_argument("NULL pointer");
  }
}

PDFTools::FILEInput::FILEInput(const char *filename)
{
  if (!filename) {
    throw invalid_argument("NULL pointer");
  }
  if ((f=fopen(filename,"r"))==NULL) {
    throw FS_except(errno);
  }
  ourclose=true;
}

PDFTools::FILEInput::~FILEInput()
{
  if (ourclose) {
    fclose(f);
  }
}

int PDFTools::FILEInput::read(char *buf,int len)
{
  return fread(buf,1,len,f);
}

long PDFTools::FILEInput::pos() const
{
  return ftell(f);
}

void PDFTools::FILEInput::pos(long pos)
{
  if (fseek(f,pos,(pos<0)?SEEK_END:SEEK_SET)!=0) {
    throw FS_except(errno);
  }
}
// }}}

// {{{ PDFTools::FILEOutput
PDFTools::FILEOutput::FILEOutput(FILE *f) : f(f),ourclose(false),sumout(0)
{
  if (!f) {
    throw invalid_argument("NULL pointer");
  }
}

PDFTools::FILEOutput::FILEOutput(const char *filename) : sumout(0)
{
  if (!filename) {
    throw invalid_argument("NULL pointer");
  }
  if ((f=fopen(filename,"w"))==NULL) {
    throw FS_except(errno);
  }
  ourclose=true;
}

// simplify  output to file, or stdout if ==NULL  case
PDFTools::FILEOutput::FILEOutput(const char *filename,FILE *_f) : sumout(0)
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
    throw invalid_argument("NULL pointer");
  }
}

PDFTools::FILEOutput::~FILEOutput()
{
  if (ourclose) {
    fclose(f);
  }
}

void PDFTools::FILEOutput::vprintf(const char *fmt,va_list ap)
{
  int ret=vfprintf(f,fmt,ap);
  if (ret<0) {
    throw FS_except(errno); // ? 
  }
  sumout+=ret;
}

void PDFTools::FILEOutput::write(const char *buf,int len)
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

long PDFTools::FILEOutput::sum() const
{
  long ret=ftell(f);
  if (ret==-1) { // fallback
    return sumout; 
  }
  return ret;
}

void PDFTools::FILEOutput::flush()
{
  fflush(f);
}
// }}}

// {{{ PDFTools::InflateInput
PDFTools::InflateInput::InflateInput(Input &read_from) : read_from(read_from)
{
  buf.resize(4096);
  
  // use defaults
  zstr.zalloc=Z_NULL;
  zstr.zfree=Z_NULL;
  zstr.opaque=Z_NULL;

//  zstr.next_in=(Bytef *)&buf[0];
//  zstr.avail_in=read_from.read(&buf[0],4096);
  zstr.next_in=NULL;
  zstr.avail_in=0;

  zstr.next_out=NULL;
  zstr.avail_out=0;

  if (inflateInit(&zstr)!=Z_OK) {
    throw UsrError("Error initializing inflate: %s",zstr.msg);
  }
}

PDFTools::InflateInput::~InflateInput()
{
  inflateEnd(&zstr);
}

int PDFTools::InflateInput::read(char *_buf,int len)
{
  int res;
  zstr.next_out=(Bytef *)_buf;
  zstr.avail_out=len;
  
  while (zstr.avail_out>0) {
    if (zstr.avail_in==0) {
      zstr.next_in=(Bytef *)&buf[0];
      zstr.avail_in=read_from.read(&buf[0],4096);
    }

    res=inflate(&zstr,Z_SYNC_FLUSH);
    if (res==Z_STREAM_END) {
      return len-zstr.avail_out;
    } else if (res!=Z_OK) {
      throw UsrError("Error: inflate failed: %s",zstr.msg);
    }
  }

  return len;
}

long PDFTools::InflateInput::pos() const
{
  // TODO: return current output position ? 
  return -1;
}

void PDFTools::InflateInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Reposition in Inflate is not supported");
  }
  zstr.avail_in=0;
  zstr.avail_out=0;
  int err=inflateReset(&zstr);
  if (err!=Z_OK) {
    throw UsrError("Error: inflateReset failed: %s",zstr.msg);
  }
  read_from.pos(0);
}
// }}}

// {{{ PDFTools::DeflateOutput
PDFTools::DeflateOutput::DeflateOutput(Output &write_to) : write_to(write_to)
{
  buf.resize(4096);
  
  // use defaults
  zstr.zalloc=Z_NULL;
  zstr.zfree=Z_NULL;
  zstr.opaque=Z_NULL;

  if (deflateInit(&zstr,9)!=Z_OK) {
    throw UsrError("Error initializing deflate: %s",zstr.msg);
  }

  zstr.next_in=NULL;
  zstr.avail_in=0;

  zstr.next_out=(Bytef *)&buf[0];
  zstr.avail_out=buf.size();
}

PDFTools::DeflateOutput::~DeflateOutput()
{
  deflateEnd(&zstr);
}

void PDFTools::DeflateOutput::write(const char *_buf,int len)
{
  int olen=0; // TODO: use this in >sum 
  
  zstr.next_in=(Bytef *)_buf;
  if (len<0) {
    zstr.avail_in=strlen(_buf);
  } else {
    zstr.avail_in=len;
  }

  while (zstr.avail_in) {
    if (!zstr.avail_out) {
      write_to.write(&buf[0],buf.size());
      olen+=buf.size();
      zstr.next_out=(Bytef *)&buf[0];
      zstr.avail_out=buf.size();
    }
    if (deflate(&zstr,Z_NO_FLUSH)!=Z_OK) {
      throw UsrError("Error: deflate failed: %s",zstr.msg);
    }
  }
}

void PDFTools::DeflateOutput::flush()
{
  int err;
  zstr.avail_in=0;

  while (1) {
    int len=buf.size()-zstr.avail_out; // in our queue!
    
    if (len) {
      write_to.write(&buf[0],len);
      zstr.next_out=(Bytef *)&buf[0];
      zstr.avail_out=buf.size();
    }
    
    err=deflate(&zstr,Z_FINISH);
    if (err==Z_STREAM_END) {
      write_to.write(&buf[0],buf.size()-zstr.avail_out);
      break;
    } else if (err!=Z_OK) {
      throw UsrError("Error: deflate failed: %s",zstr.msg);
    }
  }
  err=deflateReset(&zstr);
  if (err!=Z_OK) {
    throw UsrError("Error: deflateReset failed: %s",zstr.msg);
  }
  write_to.flush();
}
// }}}

// {{{ PDFTools::ParsingInput
PDFTools::ParsingInput::ParsingInput(Input &read_from) : read_from(read_from)
{
  prepos=0;
}

int PDFTools::ParsingInput::read(char *buf,int len)
{
  int ret=0;
  if (prepos<(int)prebuf.size()) {
    int l0=min(len,(int)prebuf.size()-prepos);
    memcpy(buf,&prebuf[prepos],l0*sizeof(char));
    prepos+=l0;
    len-=l0;
    if (len==0) {
      return l0;
    }
    buf+=l0;
    ret+=l0;
  }
  return ret+read_from.read(buf,len);
}

long PDFTools::ParsingInput::pos() const
{
  long res=read_from.pos();
  if (res==-1) {
    return res;
  }
  return res-(prebuf.size()-prepos);
}

void PDFTools::ParsingInput::pos(long pos)
{
  read_from.pos(pos);
  prepos=prebuf.size();
}

void PDFTools::ParsingInput::unread(const char *buf,int len)
{
  if (prepos>=len) {
    prepos-=len;
  } else if (prepos<(int)prebuf.size()) {
    prebuf.resize(len+(prebuf.size()-prepos));
    memmove(&prebuf[len],&prebuf[prepos],(prebuf.size()-prepos)*sizeof(char));
    prepos=0;
  } else { // prepos==prebuf.size()
    prebuf.resize(len);
    prepos=0;
  }
  // buf might point into prebuf, so stay safe
  memmove(&prebuf[prepos],buf,len*sizeof(char));
}

int PDFTools::ParsingInput::pread(char *buf,int len)
{
  int res=read(buf,len);
  if (res<=0) {
    return res;
  }
  for (int iA=0;iA<res;iA++) {
    if (buf[iA]=='%') {
      unread(buf+iA,res-iA);
      skip_comment();
      res+=read(buf+iA,len-iA);
    }
  }
  return res;
}

std::pair<const char *,int> PDFTools::ParsingInput::pread_to_delim(bool include_ws)
{
//  ??? = ignore leading ws? ... prepos 
  const int len=prebuf.size()-prepos;
  if ( (prepos>0)&&(len>0) ) {
    memmove(&prebuf[0],&prebuf[prepos],len*sizeof(char));
  }
  prebuf.resize(len);
  prepos=0;

#define READSIZE 20
  int start=0; // maybe ignore ws -> start>0
  // prepos=current read position;
  while (1) {
    // check for delim
    for (;prepos<(int)prebuf.size();prepos++) {
      if (Parser::is_delim(prebuf[prepos])) {
        if (prebuf[prepos]=='%') {
          skip_comment();
        }
        if ( (!Parser::is_space(prebuf[prepos]))||(!include_ws) ) {
          return make_pair<const char *,int>(&prebuf[start],prepos-start);
        }
      }
    }
    // read next; prepos==prebuf.size()
    prebuf.resize(prepos+READSIZE,0);
    const int r=read_from.read(&prebuf[prepos],READSIZE);
    if (r!=READSIZE) {
      prebuf.resize(prepos+r);
      if (r==0) {
        break;
      }
    }
  }
#undef READSIZE
  return make_pair<const char *,int>(&prebuf[start],prepos-start);
}

void PDFTools::ParsingInput::buffer(int prebuffer)
{
  const int len=prebuf.size()-prepos;
  if (len<prebuffer) { // prebuffering
    if ( (prepos>0)&&(len>0) ) {
      memmove(&prebuf[0],&prebuf[prepos],len*sizeof(char));
    }
    prebuf.resize(prebuffer,0);
    const int r=read_from.read(&prebuf[len],prebuffer-len);
    if (r!=prebuffer-len) {
      prebuf.resize(len+r);
    }
    prepos=0;
  }
}

bool PDFTools::ParsingInput::next(const char c,int prebuffer)
{
  buffer(prebuffer);
  if ( (prepos<(int)prebuf.size())&&(prebuf[prepos]==c) ) {
    prepos++;
    return true;
  }
  return false;
}

bool PDFTools::ParsingInput::next(const char *str,int prebuffer) 
{
  const int len=strlen(str);
  if (prebuffer>1) {
    buffer(prebuffer);
  } else {
    buffer(len);
  }
  if ( (len<=(int)prebuf.size()-prepos)&&(strncmp(&prebuf[prepos],str,len)==0) ) {
    prepos+=len;
    return true;
  }
  return false;
}

void PDFTools::ParsingInput::skip(bool required) 
{
  buffer(10);
  while (prepos<(int)prebuf.size()) {
    if (prebuf[prepos]=='%') {
      skip_comment();
    } else if (!Parser::is_space(prebuf[prepos])) {
      if (required) {
        fprintf(stderr,"Context: %.*s\n",prebuf.size()-prepos,&prebuf[prepos]);
        throw UsrError("Delimiting whitespace expected");
      }
      return;
    }
    required=false;
    if (++prepos==(int)prebuf.size()) {
      buffer(10);
    }
  }
}

int PDFTools::ParsingInput::readUInt()
{
  buffer(10);
  const int len=prebuf.size()-prepos;
  if (len==0) {
    throw UsrError("Empty number");
  } else if (!isdigit(prebuf[prepos])) {
    throw UsrError("Bad number");
  }

  int ret=0;
  for (;prepos<(int)prebuf.size();prepos++) {
    if (!isdigit(prebuf[prepos])) {
      break;
    }
    ret*=10;
    ret+=prebuf[prepos]-'0';
    if (ret<0) { // overflow
      throw UsrError("Number too big");
    }
  }
  return ret;
}

int PDFTools::ParsingInput::readInt()
{
  if (next('-',15)) {
    return -readUInt();
  }
  return readUInt();
}

float PDFTools::ParsingInput::readFrac(int intPart)
{
  // NOTE: the first character must be available ('buffer(1);') [covered in readInt: ...,15]
  float ret=0,scale=0.1;
  while (prepos<(int)prebuf.size()) {
    if (!isdigit(prebuf[prepos])) { 
      // (? '-' in middle of number allowed in adobe reader???)
      break;
    }
    ret+=(prebuf[prepos]-'0')*scale;
    scale*=0.1;
    if (++prepos==(int)prebuf.size()) {
      buffer(5);
    }
  }
  if (intPart<0) {
    return intPart-ret;
  }
  return intPart+ret;
}

void PDFTools::ParsingInput::skip_comment()
{
  int start=prepos;
  while (1) {
    for (;prepos<(int)prebuf.size();++prepos) {
      const int r=Parser::skip_eol(&prebuf[prepos]);
      if (r) {
        prepos+=r-1; // keep last char as whitespace for next token
        if (start) {
          memmove(&prebuf[start],&prebuf[prepos],(prebuf.size()-prepos)*sizeof(char));
          prebuf.resize(prebuf.size()-prepos+start);
          prepos=start;
        }
        return;
      }
    }
    // refill buffer
#define READSIZE 40
    prebuf.resize(start+READSIZE,0);
    const int r=read_from.read(&prebuf[start],READSIZE);
    if (r==0) {
      fprintf(stderr,"WARNING: comment without eol\n");
      return;
    }
    prebuf.resize(start+r,0);
#undef READSIZE
  }
}

int PDFTools::ParsingInput::read_escape() // {{{
{
  buffer(4);
  if ( (prepos+1>=(int)prebuf.size())||(prebuf[prepos]!='\\') ) {
    return -1;
  }
  prepos+=2;
  switch (prebuf[prepos-1]) {
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  case '\r':
    if ( (prepos<(int)prebuf.size())&&(prebuf[prepos]=='\n') ) {
      prepos++;
    }
    return -2; // no output
  case '\n':
    return -2;
  case '0': case '1': case '2': case '3':
  case '4': case '5': case '6': case '7':
    break;
  case '\\':
  case '(':
  case ')':
  default:
    return prebuf[prepos-1];
  }
  // now parse octal
  int ret=prebuf[prepos-1]-'0';
  if ( (prepos==(int)prebuf.size())||(prebuf[prepos]<'0')||(prebuf[prepos]>'7') ) {
    return ret;
  }
  ret<<=3;
  ret+=prebuf[prepos]-'0';
  prepos++;
  if ( (prepos==(int)prebuf.size())||(prebuf[prepos]<'0')||(prebuf[prepos]>'7') ) {
    return ret;
  }
  ret<<=3;
  ret+=prebuf[prepos]-'0';
  prepos++;
  return ret;
}
// }}}
// }}}

// {{{ PDFTools::SubInput
PDFTools::SubInput::SubInput(Input &_read_from,long _startpos,long _endpos) 
    : read_from((dynamic_cast<SubInput *>(&_read_from)==0)?_read_from:dynamic_cast<SubInput &>(_read_from).read_from),startpos(_startpos),endpos(_endpos)
{
  SubInput *rf=dynamic_cast<SubInput*>(&_read_from);
  if (rf) { // special case: SubInput of SubInput
    startpos+=rf->startpos;
    if (rf->endpos!=-1) {
      if (endpos!=-1) {
        endpos+=rf->endpos;
      }
    } else {
      endpos=-1;
    }
  }
  pos(0);
}

bool PDFTools::SubInput::empty() const 
{
  return (startpos==-1);
}

long PDFTools::SubInput::basepos() const
{
  assert(!empty());
  return startpos+cpos;
}

int PDFTools::SubInput::read(char *buf,int len)
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

long PDFTools::SubInput::pos() const
{
  return cpos;
}

void PDFTools::SubInput::pos(long pos)
{
  if (pos<0) {
    if (endpos<0) {
      assert(0);
      throw UsrError("Not supported"); // TODO?
    }
    pos+=endpos;
  } else {
    pos+=startpos;
  }
  if (  (pos<startpos)||( (endpos>=0)&&(pos>endpos) )  ) {
    throw FS_except(EINVAL);
  }
  read_from.pos(pos);
  cpos=pos-startpos;
}
// }}}

// {{{ PDFTools::MemInput
PDFTools::MemInput::MemInput(const char *buf,int len) : buf(buf),len(len),cpos(0)
{
  if (!buf) {
    throw invalid_argument("NULL pointer");
  }
}

int PDFTools::MemInput::read(char *_buf,int _len)
{
  const int clen=min(_len,len-(int)cpos);
  memcpy(_buf,buf+cpos,clen*sizeof(char));
  cpos+=clen;
  return clen;
}

long PDFTools::MemInput::pos() const
{
  return cpos;
}

void PDFTools::MemInput::pos(long pos)
{
  if (pos<0) {
    pos+=len;
  }
  if ( (pos<0)||(pos>len) ) {
    throw FS_except(EINVAL);
  }
  cpos=pos;
}

int PDFTools::MemInput::size() const
{
  return len;
}
// }}}

// {{{ PDFTools::MemIOput
PDFTools::MemIOput::MemIOput() : cpos(0)
{
}

int PDFTools::MemIOput::read(char *_buf,int len)
{
  const int clen=min((int)buf.size()-(int)cpos,len);
  memcpy(_buf,&buf[cpos],clen*sizeof(char));
  cpos+=clen;
  return clen;
}

long PDFTools::MemIOput::pos() const
{
  return cpos;
}

void PDFTools::MemIOput::pos(long pos)
{
  if (pos<0) {
    pos+=buf.size();
  }
  if ( (pos<0)||(pos>(int)buf.size()) ) {
    throw FS_except(EINVAL);
  }
  cpos=pos;
}

void PDFTools::MemIOput::write(const char *_buf,int len)
{
  if (len<0) {
    len=strlen(_buf);
  }
  buf.insert(buf.end(),_buf,_buf+len);
}

int PDFTools::MemIOput::size() const
{
  return buf.size();
}
// }}}


void PDFTools::preprint(ParsingInput &in) // {{{
{
  char buf[10];
  int res=in.read(buf,10);
  printf("\"");
  for (int iA=0;iA<res;iA++) {
    if (buf[iA]<32) {
      printf("\\x%02x",(unsigned char)buf[iA]);
    } else {
      putchar(buf[iA]);
    }
  }
  printf("\"\n");
  in.unread(buf,res);
}
// }}}

void PDFTools::dump(const Object *obj) // {{{
{
  if (!obj) { 
    printf("NULL\n");
    return;
  }
  FILEOutput fo(stdout);
  obj->print(fo);
  fo.put('\n');
}
// }}}


/*
  class Stream { // a stream is not an object // TODO: a stream is an object
  public:
    Stream(Output &
    virtual ~Stream() {}

    ... setOutput, finishOutput, ? RAII 
  };
  class IStream : public Stream, public Input {
  public:
    IStream(InputPDFBase &in); ... may require to resolve indirect Objects...
    virtual ~IStream() {}

    virtual long len() const=0;

    const Object *find_dict(const char *key) const;
  private:
    Dict streamdict;
  };
  class OStream : public Stream, public Output {
  public:
    OStream(OutputPDFBase &out); ... getRef... more than one PDF Object?
    virtual ~OStream(); ... finishOutput
    ... getRef... 

    void add_dict(const char *key,const Object *obj,bool take=false);
    void add_filter(const Object *obj,bool take=false);
  private:
    Dict streamdict;
    Array filter;
  };
  template <typename T>
  class InputStream : public IStream {
  public:
    virtual ~InputStream() {}
    InputStream(T in) : in(in) {}
    InputStream(T in,long start,long end) {} ...
  private:
    T in;
  };
  template <typename T>
  class OutputStream : public OStream {
  public:
    virtual ~OutputStream() {}
  private:
    T out;
  };
  typedef InputStream<FILEInput> FILEInputStream;
  typedef OutputStream<FILEOutput> FILEOutputStream;
  typedef InputStream<MemoryInput> MemoryInputStream;
  typedef OutputStream<MemoryOutput> MemoryOutputStream;

  class InputFlateStream<InflateInput> <FILEInput>

  class StaticMemoryInput : public Input { // will not free
  private:
    const char *mem;
    int mlen;
  };
  class FixedMemoryInput : public Input { // will free
  private:
    char *mem;
    int mlen;
  };
  class VectorMemoryInput : public Input { // will delete
  private:
    std::vector<char> *mem;
  };
  class VectorMemoryOutput : public Output { // uses C++ / STL mem
  private:
  };
  class FixedMemoryOutput : public Output { // will not enlarge
  private:
  };
  class ReallocMemoryOutput : public Output { // uses C mem
  private:
  };

  class MemoryInputStream : public IStream {
  public:
    virtual ~MemoryInputStream() {}
  private:
    const char *mem;
    int mlen;
  };
  class MemoryOutputStream : public OStream {
  public:
    virtual ~MemoryOutputStream() {}
  private:
    std::vector<char> mem;
  };
*/

/*
TODO: Stream as PDF-Object: (...parseStream)
  - Stream Dictionary
  - Filters Array
  - Stream Data

  FILEInputStream
  FILEInputSubstream
  FILEOutputStream
  FILEOutputSubstream
  MemoryInputStream
  MemoryInputSubstream
  MemoryOutputStream
  MemoryOutputSubstream
*/
