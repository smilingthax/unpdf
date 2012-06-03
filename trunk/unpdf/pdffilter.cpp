#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "pdffilter.h"
#include "pdffilter_int.h"
#include "pdfio.h"
#include "pdfbase.h"
#include "pdfparse.h"
#include "exception.h"
#include <typeinfo> // TODO? typename into Object
#include <stdio.h>

#include "lzwcode.h"
#include "g4code.h"

#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>

// http://jbig2dec.sourceforge.net/ , http://www.imperialviolet.org/jbig2.html
// http://code.google.com/p/sumatrapdf/source (http://www.koders.com/c/fid48566E7D9AC20C5B1D0FDE836A80C726E318DDF5.aspx)

using namespace PDFTools;
using namespace std;

// helper to wrap a single value into an Array to simplify processing
// resolve all references
void fetch_in_array(Array &ret,PDF &pdf,const Object *obj) // {{{
{
  ObjectPtr optr=pdf.fetch(obj);
  Array *aval=dynamic_cast<Array *>(optr.get());
  if (aval) {
    if (optr.owns()) { // may modify -> take ownership
      for (int iA=0;iA<(int)aval->size();iA++) {
        ObjectPtr oqtr=aval->getTake(pdf,iA);
        ret.add(oqtr.release(),oqtr.owns());
      }
    } else { // may not modify, but resolve. Ownership of direct objects stays at >obj !!! (long enough lifetime?)
      for (int iA=0;iA<(int)aval->size();iA++) {
        ObjectPtr oqtr=aval->get(pdf,iA);
        ret.add(oqtr.release(),oqtr.owns());
      }
    }
  } else { // wrap in
    ret.add(optr.release(),optr.owns());
  }
}
// }}}

// {{{ readfunc_Input, writefunc_Output
extern "C" {
int readfunc_Input(void *user,unsigned char *buf,int len)
{
  Input *in=(Input *)user;
  int ret=in->read((char *)buf,len);
  if (ret!=len) {
    return -1;
  }
  return 0;
}

int writefunc_Output(void *user,unsigned char *buf,int len)
{
  Output *out=(Output *)user;
  out->write((char *)buf,len);
  return 0;
}
};
// }}}

// {{{ PDFTools::IFilter
PDFTools::IFilter::IFilter(PDF &pdf,const Object &filterspec,const Object *decode_params) : latein(NULL,false)
{
  // deal with /Filter
  fetch_in_array(filter,pdf,&filterspec);

  // deal with /DecodeParms
  if (decode_params) {
    fetch_in_array(params,pdf,decode_params);
  } // else empty

  try {
    init(filter,params,latein);
  } catch (...) {
    for (int iA=0;iA<(int)filter_chain.size()-1;iA++) {
      delete filter_chain[iA];
    }
    throw;
  }
}

void PDFTools::IFilter::init(const Array &filterspec,const Array &decode_params,Input &read_from)
{
  if ( (decode_params.size())&&(decode_params.size()!=filterspec.size()) ) {
    throw UsrError("/DecodeParms does not match /Filter");
  }
  const int len=filterspec.size();

  filter_chain.resize(len+1,NULL);
  filter_chain[len]=&read_from;
  for (int iA=len-1,iB=0;iA>=0;iA--,iB++) {
    const Name *nval=dynamic_cast<const Name *>(filterspec[iB]);
    if (!nval) {
      throw UsrError("Entry in /Filter is not a Name");
    }
    if (strcmp(nval->value(),"Crypt")==0) {
      if (iA!=len-1) {
        throw UsrError("Crypt filter must be first");
      }
    }
   
    const Dict *dval=NULL;
    if (decode_params.size()) {
      if (!decode_params[iB]) {
        throw invalid_argument("NULL pointer");
      }
      dval=dynamic_cast<const Dict *>(decode_params[iB]);
      if ( (!dval)&&(!isnull(decode_params[iB])) ) {
        throw UsrError("Entry in /DecodeParms is neither null nor a Dictionary");
      }
    }

    ( (filter_chain[iA]=AHexFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 )  ||
    ( (filter_chain[iA]=A85Filter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 )   ||
    ( (filter_chain[iA]=LZWFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 )   ||
    ( (filter_chain[iA]=FlateFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 ) ||
    ( (filter_chain[iA]=RLEFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 )   ||
    ( (filter_chain[iA]=FaxFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 )   ||
    ( (filter_chain[iA]=JBIG2Filter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 ) ||
    ( (filter_chain[iA]=JpegFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 )  ||
    ( (filter_chain[iA]=JPXFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 )   ||
    ( (filter_chain[iA]=CryptFilter::makeInput(nval->value(),*filter_chain[iA+1],dval))!=0 );

    if (!filter_chain[iA]) {
//      throw UsrError("Unsupported Filter: /%s",nval->value());
      fprintf(stderr,"WARNING: Unsupported Filter: /%s\n",nval->value());
      // set no filter
      filter_chain[0]=filter_chain[len];
      for (iA++;iA<(int)filter_chain.size()-1;iA++) {
        delete filter_chain[iA];
      }
      filter_chain.resize(1);
      return;
    }
  }
}

void PDFTools::IFilter::lateCloseFunc(Input *in,void *user)
{
  InputPtr *ptr=(InputPtr *)user;
  ptr->reset(NULL,false);
}

PDFTools::IFilter::~IFilter()
{
  for (int iA=0;iA<(int)filter_chain.size()-1;iA++) {
    delete filter_chain[iA];
  }
}

InputPtr PDFTools::IFilter::open(Input *read_from,bool take)
{
  if (!latein.empty()) {
    throw UsrError("Only one active reader is allowed on Filter");
  }
  latein.reset(read_from,take);
  // reset filters
  filter_chain[0]->pos(0);

  return InputPtr(filter_chain[0],false,lateCloseFunc,&latein);
}

int PDFTools::IFilter::hasBpp() const
{
  if (dynamic_cast<const FaxFilter::FInput *>(filter_chain.back())) {
    return 1;
/*  } else if (dynamic_cast<const JBIG2Filter::FInput *>(filter_chain.back())) {
    return 1;*/
  } else if (dynamic_cast<const RLEFilter::FInput *>(filter_chain.back())) {
    return 8;
  } else if (dynamic_cast<const JpegFilter::FInput *>(filter_chain.back())) {
    return 8;
/*  } else if (dynamic_cast<const LZWFilter::Predictor *>(filter_chain.back())) {
    return ...; predictor.bpp*/
/*  } else if (dynamic_cast<const JPXFilter::FInput *>(filter_chain.back())) {
    return ...; filter.bpp*/
  }
  return -1;
}

bool PDFTools::IFilter::isJPX() const
{
  return false;
//  return (dynamic_cast<const JPXFilter::FInput *>(filter_chain.back())!=NULL);
}

bool PDFTools::IFilter::isJPX(ColorSpace &cs) const
{
/*
  if (JPXFilter::FInput *fx=dynamic_cast<const JPXFilter::FInput *>(filter_chain.back())) {
    cs=fx.cs;
    return true;
  }
*/
  return false;
}
// }}}

// {{{ PDFTools::OFilter
PDFTools::OFilter::OFilter() : lateout(NULL,false)
{
  filter_chain.push_back(&lateout);
}

PDFTools::OFilter::~OFilter()
{
  for (int iA=filter_chain.size()-1;iA>=1;iA--) {
    delete filter_chain[iA];
  }
}

const Object *PDFTools::OFilter::getFilter() const
{
  if (!filter.size()) {
    return NULL;
  } else if (filter.size()==1) {
    return filter[0];
  } else {
    return &filter;
  }
}

const Object *PDFTools::OFilter::getParams() const
{
  int iA=0;
  for (;iA<(int)params.size();iA++) {
    if (!isnull(params[iA])) {
      break;
    }
  }
  if (iA==(int)params.size()) { // only null
    return NULL;
  } else if (filter.size()==1) {
    return params[0];
  } else {
    return &params;
  }
}

OutputPtr PDFTools::OFilter::open(Output &write_to)
{
  if (!lateout.empty()) {
    throw UsrError("Only one active writer is allowed on Filter");
  }
  lateout.reset(&write_to,false);
  // ... "reset" is done on flush
  return OutputPtr(filter_chain.back(),false);
}

Output &PDFTools::OFilter::getOutput()
{
  return *filter_chain.back();
}

void PDFTools::OFilter::addFilter(const char *name,Dict *_params,Output *out)
{
  if (strcmp(name,"Crypt")==0) {
    if (filter.size()!=0) {
      throw UsrError("Crypt filter must be first");
    }
  }
  filter.add(new Name(name,Name::STATIC),true);
  if (_params) {
    params.add(_params,true);
  } else {
    params.add(new Object(),true);
  }
  filter_chain.push_back(out);
}

bool PDFTools::OFilter::isJPX() const
{
  return false;
//  return (dynamic_cast<const JPXFilter::FInput *>(filter_chain.back())!=NULL);
}
// }}}

// {{{ AHexFilter  - ASCIIHexDecode
const char *AHexFilter::name="ASCIIHexDecode";

#define BLOCK_SIZE 4096
// {{{ AHexFilter::FInput
AHexFilter::FInput::FInput(Input &read_from) : read_from(read_from)
{
  reset();
}

int AHexFilter::FInput::read(char *buf,int len)
{
  int olen=0;
  bool second=false;

  if (len==0) {
    return 0;
  }
  while (1) {
    for (;inpos<(int)block.size();inpos++) {
      if (isxdigit(block[inpos])) {
        if (second) {
          *buf|=Parser::one_hex(block[inpos]);
          buf++;
          len--;
          olen++;
          if (len==0) {
            inpos++;
            return olen;
          }
        } else {
          *buf=Parser::one_hex(block[inpos])<<4;
        }
        second=!second;
      } else if (block[inpos]=='>') { // done
        return olen+second;
      } else if (!Parser::is_space(block[inpos])) {
        throw UsrError("Bad Hexstream");
      }
    }
    block.resize(BLOCK_SIZE);
    int res=read_from.read(&block[0],block.size());
    if (res==0) {
      throw UsrError("Hexstream not terminated");
    }
    block.resize(res);
    inpos=0;
  }
}

void AHexFilter::FInput::reset()
{
  block.resize(BLOCK_SIZE);
  inpos=block.size();
}

long AHexFilter::FInput::pos() const
{
  return -1;
}

void AHexFilter::FInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Reposition in AHexFilter_Input is not supported");
  }
  reset();
  read_from.pos(0);
}
// }}}
#undef BLOCK_SIZE

#define LINES_MAX 70
// {{{ AHexFilter::FOutput
AHexFilter::FOutput::FOutput(Output &write_to) : write_to(write_to)
{
  block.reserve(LINES_MAX+1);
}

void AHexFilter::FOutput::write(const char *buf,int len)
{
  const char hexchar[]="0123456789abcdef";
  if (len<0) {
    len=strlen(buf);
  }

  while (1) {
    for (;(int)block.size()<LINES_MAX;len--,buf++) {
      if (len==0) {
        return;
      }
      block.push_back(hexchar[(*buf>>4)&0x0f]);
      block.push_back(hexchar[(*buf)&0x0f]);
    }
    block.push_back('\n');
    write_to.write(&block[0],block.size());
    block.clear();
  }
}

void AHexFilter::FOutput::flush()
{
  block.push_back('>');
  block.push_back('\n');
  write_to.write(&block[0],block.size());
  block.clear();
  write_to.flush();
}
// }}}
#undef LINES_MAX

Input *AHexFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }
  return new FInput(read_from); // not implemented
}

void AHexFilter::makeOutput(OFilter &filter)
{
  filter.addFilter(name,NULL,new FOutput(filter.getOutput()));
}
// }}}

// {{{ A85Filter   - ASCII85Decode
const char *A85Filter::name="ASCII85Decode";

#define BLOCK_SIZE 4096
// {{{ A85Filter::FInput
A85Filter::FInput::FInput(Input &read_from) : read_from(read_from)
{
  reset();
}

static inline int OUT4(char *&buf,int &len,const unsigned int &ubuf,int &ulen)
{
  if (len<ulen) {
    const int ret=len;
    for (;len>0;len--,buf++,ulen--) {
      *buf=(ubuf>>(8*ulen-8))&0xff;
    }
    return ret;
  } else {
    const int ret=ulen;
    for (;ulen>0;buf++,ulen--) {
      *buf=(ubuf>>(8*ulen-8))&0xff;
    }
    len-=ret;
    return ret;
  }
}

int A85Filter::FInput::read(char *buf,int len)
{
  const unsigned int pow85[]={85*85*85*85, 85*85*85, 85*85, 85, 1};
  int olen=0;
  unsigned int rbuf=0;
  int rlen=0;

  olen+=OUT4(buf,len,ubuf,ulen);
  if (!len) {
    return olen;
  }
  while (1) {
    for (;inpos<(int)block.size();inpos++) {
      // ulen==0
      if ( (block[inpos]>=33)&&(block[inpos]<=119) ) { // is85digit
        const unsigned int add=(block[inpos]-33)*pow85[rlen++];
        if (rbuf>UINT_MAX-add) {
          throw UsrError("Overflow in ASCII85");
        }
        rbuf+=add;
        if (rlen==5) {
          ubuf=rbuf;
          ulen=4;
          rlen=0;
          rbuf=0;
          olen+=OUT4(buf,len,ubuf,ulen);
          if (!len) {
            inpos++;
            return olen;
          }
        }
      } else if ( (rlen==0)&&(block[inpos]=='z') ) {
        ubuf=0;
        ulen=4;
        olen+=OUT4(buf,len,ubuf,ulen);
        if (!len) {
          inpos++;
          return olen;
        }
      } else if (block[inpos]=='~') { // done
        if (inpos==(int)block.size()-1) {
          block.resize(block.size()+1);
          int res=read_from.read(&block[inpos+1],1);
          if (!res) {
            block[inpos+1]=0;
          }
        }
        if (block[inpos+1]!='>') {
          throw UsrError("Bad terminator in ASCII85 stream");
        }
        if (rlen>=2) {
          ulen=rlen-1;
          ubuf=(rbuf+pow85[rlen-1])>>(8*(4-ulen)); // rounding!
        } else if (rlen!=0) {
          throw UsrError("Bad trailing byte in ASCII85 stream");
        }
        olen+=OUT4(buf,len,ubuf,ulen);
        return olen;
      } else if (!Parser::is_space(block[inpos])) {
        throw UsrError("Bad ASCII85 stream");
      }
    }
    block.resize(BLOCK_SIZE);
    int res=read_from.read(&block[0],block.size());
    if (res==0) {
      throw UsrError("ASCII85 stream not terminated");
    }
    block.resize(res);
    inpos=0;
  }
}

void A85Filter::FInput::reset()
{
  block.resize(BLOCK_SIZE);
  inpos=block.size();
  ubuf=0;
  ulen=0;
}

long A85Filter::FInput::pos() const
{
  return -1;
}

void A85Filter::FInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Reposition in A85Filter_Input is not supported");
  }
  reset();
  read_from.pos(0);
}
// }}}
#undef BLOCK_SIZE

#define LINES_LEN 70
// {{{ A85Filter::FOutput
static inline void ENC5(unsigned int a,char *buf) 
{
  for (int iA=4;iA>=0;iA--) {
    buf[iA]=(a%85)+33;
    a/=85;
  }
}

A85Filter::FOutput::FOutput(Output &write_to) : write_to(write_to),ubuf(0),ulen(0),linelen(0)
{
}

void A85Filter::FOutput::write(const char *buf,int len)
{
  if (len<0) {
    len=strlen(buf);
  }
  char obuf[5];

  const int cend=max(len-(4-ulen),0);
  for (;len>cend;len--,buf++,ulen++) {
    ubuf|=((unsigned char)*buf<<(24-8*ulen));
  }
  if (ulen<4) {
    return;
  }

  while (1) {
    if (!ubuf) {
      write_to.put('z');
      if (++linelen==LINES_LEN) {
        write_to.put('\n');
        linelen=0;
      }
    } else {
      ENC5(ubuf,obuf);
      if (linelen+5>=LINES_LEN) {
        write_to.write(obuf,LINES_LEN-linelen);
        write_to.put('\n');
        write_to.write(obuf+(LINES_LEN-linelen),5-(LINES_LEN-linelen));
        linelen=5-(LINES_LEN-linelen);
      } else {
        write_to.write(obuf,5);
        linelen+=5;
      }
    }
    if (len<4) {
      break;
    }
    ubuf=((unsigned char)buf[0]<<24)|((unsigned char)buf[1]<<16)|
         ((unsigned char)buf[2]<<8)|((unsigned char)buf[3]);
    buf+=4;
    len-=4;
  }
  ubuf=0;
  ulen=len;
  for (int iA=24;len>0;len--,buf++,iA-=8) {
    ubuf|=((unsigned char)*buf<<iA);
  }
}

void A85Filter::FOutput::flush()
{
  char obuf[5];
  if (ulen) {
    if (linelen+ulen+1+2>LINES_LEN) {
      write_to.put('\n');
    }
    ENC5(ubuf,obuf);
    write_to.write(obuf,ulen+1);
    ubuf=0;
    ulen=0;
  }
  write_to.puts("~>\n");
  linelen=0;
  write_to.flush();
}
// }}}
#undef LINES_LEN

Input *A85Filter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }
  return new FInput(read_from);
}

void A85Filter::makeOutput(OFilter &filter)
{
  filter.addFilter(name,NULL,new FOutput(filter.getOutput()));
}
// }}}

// {{{ Pred{Input,Output}
// {{{ PDFTools::PredInput
PDFTools::PredInput::PredInput(Input *read_from,int width,int color,int bpp,int predictor) :
  read_from(read_from),
  pbyte((color*bpp+7)/8),
  pshift((color*bpp)%8),
  ispng(predictor>=10)
{
  const int bwidth=(color*bpp*width+7)/8;
  try {
    assert(read_from);
    assert(  (predictor==2)||( (predictor>=10)&&(predictor<=15) )  );

    line[0].resize(bwidth+pbyte,0);
    thisline=&line[0][0];
    if (ispng) {
      line[1].resize(bwidth+pbyte,0);
      lastline=&line[1][0];
    } else {
      lastline=NULL;
    }
    cpos=bwidth+pbyte;
  } catch (...) {
    delete read_from;
    throw;
  }
}

PDFTools::PredInput::~PredInput()
{
  delete read_from;
}

void PDFTools::PredInput::tiff_decode()
{
  if (bpp==1) {
    for (int iA=pbyte;iA<(int)line[0].size();iA++) {
      thisline[iA]^=((thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1])>>pshift;
    }
  } else if (bpp==2) {
    for (int iA=pbyte;iA<(int)line[0].size();iA++) {
      const unsigned char prev=((thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1])>>pshift;
      thisline[iA]=(((thisline[iA]&0xcc)+(prev&0xcc))&0xcc)|
                   (((thisline[iA]&0x33)+(prev&0x33))&0x33);
    }
  } else if (bpp==4) {
    for (int iA=pbyte;iA<(int)line[0].size();iA++) {
      const unsigned char prev=((thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1])>>pshift;
      thisline[iA]=(((thisline[iA]&0xf0)+(prev&0xf0))&0xf0)|
                   (((thisline[iA]&0x0f)+(prev&0x0f))&0x0f);
    }
  } else if (bpp==8) {
    for (int iA=pbyte;iA<(int)line[0].size();iA++) {
      thisline[iA]+=thisline[iA-pbyte];
    }
  } else if (bpp==16) {
    for (int iA=pbyte;iA<(int)line[0].size();iA+=2) {
      int c=(thisline[iA]<<8)|thisline[iA+1];
      c-=(thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1];
      thisline[iA]=c>>8;
      thisline[iA+1]=c&0xff;
    }
  }
}

void PDFTools::PredInput::png_decode(int type)
{
  if (type==0) { // none
  } else if (type==1) { // sub
    for (int iA=color;iA<(int)line[0].size();iA++) {
      thisline[iA]+=thisline[iA-color];
    }
  } else if (type==2) { // up
    for (int iA=color;iA<(int)line[0].size();iA++) {
      thisline[iA]+=lastline[iA];
    }
  } else if (type==3) { // avg
    for (int iA=color;iA<(int)line[0].size();iA++) {
      thisline[iA]+=(thisline[iA-color]+lastline[iA])/2; // floor
    }
  } else if (type==4) { // paeth
    for (int iA=color;iA<(int)line[0].size();iA++) {
      const int p=thisline[iA-color]+lastline[iA]-lastline[iA-color];
      const int pa=abs(p-thisline[iA-color]),
                pb=abs(p-lastline[iA]),
                pc=abs(p-lastline[iA-color]);
      if ( (pa<=pb)&&(pa<=pc) ) {
        thisline[iA]+=thisline[iA-color];
      } else if (pb<=pc) {
        thisline[iA]+=lastline[iA];
      } else {
        thisline[iA]+=lastline[iA-color];
      }
    }
  } else {
    assert(0);
  }
}

int PDFTools::PredInput::read(char *buf,int len)
{
  int olen=0;
  while (len>0) {
    if (cpos!=(int)line[0].size()) {
      const int clen=min(len,(int)line[0].size()-cpos);
      memcpy(buf,thisline+cpos,clen*sizeof(char));
      cpos+=clen;
      olen+=clen;
      if (len==0) {
        return olen;
      }
      buf+=clen;
      len-=clen;
    }
    if (ispng) {
      int res=read_from->read((char *)thisline+pbyte-1,line[0].size()-pbyte+1);
      if (res==0) {
        return olen;
      } else if (res!=(int)line[0].size()-pbyte+1) {
        throw UsrError("Incomplete line");
      }
      swap(thisline,lastline);
      const char pred=thisline[pbyte-1];
      thisline[pbyte-1]=0;
      png_decode(pred);
    } else {
      int res=read_from->read((char *)thisline+pbyte,line[0].size()-pbyte);
      if (res==0) {
        return olen;
      } else if (res!=(int)line[0].size()-pbyte) {
        throw UsrError("Incomplete line");
      }
      tiff_decode();
    }
    cpos=pbyte;
  }
  return len;
}

long PDFTools::PredInput::pos() const
{
  return -1;
}

void PDFTools::PredInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Reposition in PredInput is not supported");
  }
  thisline=&line[0][0];
  if (ispng) {
    lastline=&line[1][0];
    memset(lastline,0,line[0].size()*sizeof(char));
  }
  cpos=line[0].size();

  read_from->pos(0);
}
// }}}

// {{{ PDFTools::PredOutput
PDFTools::PredOutput::PredOutput(Output *write_to,int width,int color,int bpp,int predictor) :
  write_to(write_to),
//  width(width),
  color(color),bpp(bpp),
  pbyte((color*bpp+7)/8),
  pshift((color*bpp)%8),
  ispng(predictor>=10)
{
  const int bwidth=(color*bpp*width+7)/8;
  try {
    assert(write_to);
    assert(  (predictor==2)||( (predictor>=10)&&(predictor<=15) )  );

    line[0].resize(bwidth+pbyte,0);
    thisline=&line[0][0];
    if (ispng) {
      line[1].resize(bwidth+pbyte,0);
      lastline=&line[1][0];
      encd.resize(4);
      encd[0].resize(bwidth+1);
      encd[0][0]=1;
      encd[1].resize(bwidth+1);
      encd[1][0]=2;
      encd[2].resize(bwidth+1);
      encd[2][0]=3;
      encd[3].resize(bwidth+1);
      encd[3][0]=4;
    } else {
      lastline=NULL;
    }
    cpos=pbyte;
  } catch (...) {
    delete write_to;
    throw;
  }
}

PDFTools::PredOutput::~PredOutput()
{
  delete write_to;
}

void PDFTools::PredOutput::tiff_encode()
{
  if (bpp==1) {
    for (int iA=line[0].size()-1;iA>=pbyte;iA--) {
      thisline[iA]^=((thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1])>>pshift;
    }
  } else if (bpp==2) {
    for (int iA=line[0].size()-1;iA>=pbyte;iA--) {
      const unsigned char prev=((thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1])>>pshift;
      thisline[iA]=(((thisline[iA]&0xcc)+0x10-(prev&0xcc))&0xcc)|
                   (((thisline[iA]&0x33)+0x44-(prev&0x33))&0x33);
    }
  } else if (bpp==4) {
    for (int iA=line[0].size()-1;iA>=pbyte;iA--) {
      const unsigned char prev=((thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1])>>pshift;
      thisline[iA]=(((thisline[iA]&0xf0)-(prev&0xf0))&0xf0)|
                   (((thisline[iA]&0x0f)-(prev&0x0f))&0x0f);
    }
  } else if (bpp==8) {
#if 0
    for (int iA=line[0].size()-1;iA>=pbyte;iA--) {
      thisline[iA]-=thisline[iA-pbyte];
    }
#else
    unsigned char *dst=thisline+line[0].size()-1;
    const unsigned char *src=dst-pbyte;
    for (;src>=thisline;--dst,--src) {
      *dst-=*src;
    }
#endif
  } else if (bpp==16) {
    for (int iA=line[0].size()-2;iA>=pbyte;iA-=2) {
      int c=(thisline[iA]<<8)|thisline[iA+1];
      c-=(thisline[iA-pbyte]<<8)|thisline[iA-pbyte+1];
      thisline[iA]=c>>8;
      thisline[iA+1]=c&0xff;
    }
  }
}

// lastline has to be uncoded previous line 
void PDFTools::PredOutput::png_encode(unsigned char *dest,int type)
{
  dest+=line[0].size()-pbyte;
  if (type==0) { // none
    assert(0);
    memcpy(dest-line[0].size()+pbyte,thisline,(line[0].size()-pbyte)*sizeof(char));
  } else if (type==1) { // sub
    for (int iA=line[0].size()-1;iA>=color;iA--) {
      *--dest=thisline[iA]-thisline[iA-color];
    }
  } else if (type==2) { // up
    for (int iA=line[0].size()-1;iA>=color;iA--) {
      *--dest=thisline[iA]-lastline[iA];
    }
  } else if (type==3) { // avg
    for (int iA=line[0].size()-1;iA>=color;iA--) {
      *--dest=thisline[iA]-(thisline[iA-color]+lastline[iA])/2; // floor
    }
  } else if (type==4) { // paeth
    for (int iA=line[0].size()-1;iA>=color;iA--) {
      const int p=thisline[iA-color]+lastline[iA]-lastline[iA-color];
      const int pa=abs(p-thisline[iA-color]),
                pb=abs(p-lastline[iA]),
                pc=abs(p-lastline[iA-color]);
      if ( (pa<=pb)&&(pa<=pc) ) {
        *--dest=thisline[iA]-thisline[iA-color];
      } else if (pb<=pc) {
        *--dest=thisline[iA]-lastline[iA];
      } else {
        *--dest=thisline[iA]-lastline[iA-color];
      }
    }
  }
}

int PDFTools::PredOutput::png_encode()
{
/* TODO
  if (bpp<8) { // fixed
    const int type=0;
    if (type!=0) {
      png_encode(&encd[type-1][1],type);
    }
    return type;
  }
*/
  unsigned char *d1=&encd[0][1]+line[0].size()-pbyte,
                *d2=&encd[1][1]+line[0].size()-pbyte,
                *d3=&encd[2][1]+line[0].size()-pbyte,
                *d4=&encd[3][1]+line[0].size()-pbyte;
  int a[5]={0,0,0,0,0};
  for (int iA=line[0].size()-1;iA>=color;iA--) {
    *--d1=thisline[iA]-thisline[iA-color];
    *--d2=thisline[iA]-lastline[iA];
    *--d3=thisline[iA]-(thisline[iA-color]+lastline[iA])/2; // floor
    const int p=thisline[iA-color]+lastline[iA]-lastline[iA-color];
    const int pa=abs(p-thisline[iA-color]),
              pb=abs(p-lastline[iA]),
              pc=abs(p-lastline[iA-color]);
    if ( (pa<=pb)&&(pa<=pc) ) {
      *--d4=thisline[iA]-thisline[iA-color];
    } else if (pb<=pc) {
      *--d4=thisline[iA]-lastline[iA];
    } else {
      *--d4=thisline[iA]-lastline[iA-color];
    }
    a[0]+=(char)thisline[iA];
    a[1]+=*(char *)d1;
    a[2]+=*(char *)d2;
    a[3]+=*(char *)d3;
    a[4]+=*(char *)d4;
  }
  // find best
  int ret=0;
  for (int iA=1;iA<5;iA++) {
    if (a[iA]<a[ret]) {
      ret=iA;
    }
  }
  return ret;
}

void PDFTools::PredOutput::write(const char *buf,int len)
{
  if (len<0) {
    len=strlen(buf);
  }
  while (len>0) {
    const int clen=min(len,(int)line[0].size()-cpos);
    memcpy(thisline+cpos,buf,clen*sizeof(char));
    cpos+=clen;
    if (cpos<(int)line[0].size()) {
      return;
    }
    buf+=clen;
    len-=clen;

    if (ispng) {
      const int type=png_encode();
      if (type==0) {
        write_to->write((const char *)thisline+pbyte-1,encd[0].size());
      } else {
        write_to->write((const char *)&encd[type-1][0],encd[0].size());
      }
      swap(lastline,thisline);
    } else {
      tiff_encode();
      write_to->write((const char *)thisline+pbyte,line[0].size()-pbyte);
    }
    cpos=pbyte;
  }
}

void PDFTools::PredOutput::flush()
{
  thisline=&line[0][0];
  if (ispng) {
    lastline=&line[1][0];
    memset(lastline,0,line[0].size()*sizeof(char));
  }
  cpos=pbyte;

  write_to->flush();
}
// }}}
// }}}

// {{{ LZWFilter   - LZWDecode
const char *LZWFilter::name="LZWDecode";

// {{{ LZWFilter::Params
LZWFilter::Params::Params() : predictor(1),color(1),bpc(8),width(1),early(1)
{
}

LZWFilter::Params::Params(const Dict &params) : color(1),bpc(8),width(1)
{
  predictor=params.getInt_D("Predictor",1);

  if (predictor!=1) {
    color=params.getInt_D("Colors",1);
    bpc=params.getInt_D("BitsPerComponent",8);
    width=params.getInt_D("Columns",1);
  }

  early=params.getInt_D("EarlyChange",1);
}

Dict *LZWFilter::Params::getDict() const
{
  if ( (early==1)&&(predictor==1) ) {
    return NULL;
  }
  auto_ptr<Dict> ret(new Dict);

  if (predictor!=1) {
    ret->add("Predictor",predictor);
    
    if (color!=1) {
      ret->add("Colors",color);
    }
    if (bpc!=8) {
      ret->add("BitsPerComponent",bpc);
    }
    if (width!=1) {
      ret->add("Columns",width);
    }
  }
  if (early!=1) {
    ret->add("EarlyChange",early);
  }

  return ret.release();
}
// }}}

// {{{ LZWFilter::FInput
LZWFilter::FInput::FInput(Input &read_from,int early) : read_from(read_from),eof(false)
{
  state=init_lzw_read(early,readfunc_Input,&read_from);
  if (!state) {
    throw bad_alloc();
  }
}

LZWFilter::FInput::~FInput() 
{
  free_lzw(state);
}

int LZWFilter::FInput::read(char *buf,int len)
{
  if (eof) {
    return 0;
  }
  int res=decode_lzw(state,(unsigned char*)buf,len);
  if (res>0) {
    eof=true;
    return res-1; // EOF
  } else if (res<0) {
    throw UsrError("decode_lzw failed: %d",res);
  }
  return len;
}

long LZWFilter::FInput::pos() const
{
  return -1;
}

void LZWFilter::FInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Reposition in LZWFilter_Input is not supported");
  }
  eof=false;
  restart_lzw(state);
  read_from.pos(0);
}
// }}}

// {{{  LZWFilter::FOutput
LZWFilter::FOutput::FOutput(Output &write_to,int early) : write_to(write_to)
{
  state=init_lzw_write(early,writefunc_Output,&write_to);
  if (!state) {
    throw bad_alloc();
  }
}

LZWFilter::FOutput::~FOutput()
{
  free_lzw(state);
}

void LZWFilter::FOutput::write(const char *buf,int len)
{
  if (len<0) {
    len=strlen(buf);
  }
  int res=encode_lzw(state,(unsigned char *)buf,len);
  if (res!=0) {
    throw UsrError("encode_lzw failed: %d",res);
  }
}

void LZWFilter::FOutput::flush()
{
  int res=encode_lzw(state,NULL,0);
  if (res!=0) {
    throw UsrError("encode_lzw failed: %d",res);
  }
  restart_lzw(state);
  write_to.flush();
}
// }}}

Input *LZWFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }

  if (params) {
    Params prm(*params);

    if (prm.predictor!=1) {
      return new PredInput(new FInput(read_from,prm.early),prm.width,prm.color,prm.bpc,prm.predictor);
    }
    return new FInput(read_from,prm.early);
  } else {
    return new FInput(read_from,1);
  }
}

void LZWFilter::makeOutput(OFilter &filter,const Params &prm)
{
  if (prm.predictor!=1) {
    filter.addFilter(name,prm.getDict(),new PredOutput(new FOutput(filter.getOutput(),prm.early),prm.width,prm.color,prm.bpc,prm.predictor));
  } else {
    filter.addFilter(name,prm.getDict(),new FOutput(filter.getOutput(),prm.early));
  }
}
// }}}

// {{{ FlateFilter - FlateDecode
const char *FlateFilter::name="FlateDecode";

// {{{ FlateFilter::FInput
FlateFilter::FInput::FInput(Input &read_from) : read_from(read_from)
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

FlateFilter::FInput::~FInput()
{
  inflateEnd(&zstr);
}

int FlateFilter::FInput::read(char *_buf,int len)
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

long FlateFilter::FInput::pos() const
{
  // TODO: return current output position ? 
  return -1;
}

void FlateFilter::FInput::pos(long pos)
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

// {{{ FlateFilter::FOutput
FlateFilter::FOutput::FOutput(Output &write_to) : write_to(write_to)
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

FlateFilter::FOutput::~FOutput()
{
  deflateEnd(&zstr);
}

void FlateFilter::FOutput::write(const char *_buf,int len)
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

void FlateFilter::FOutput::flush()
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

Input *FlateFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }

  if (params) {
    Params prm(*params);

    if (prm.predictor!=1) {
      return new PredInput(new FInput(read_from),prm.width,prm.color,prm.bpc,prm.predictor);
    }
  }
    /*
      // return FInput("",param1,param2,param3...);
      return PngInput("",param1,param2,param3...);
    */
  return new FInput(read_from);
  /* [PNG]
    ... png: export_native();
  */
}

void FlateFilter::makeOutput(OFilter &filter,const Params &prm)
{
  if (prm.early!=1) {
    throw UsrError("/EarlyChange not valid for /FlateDecode");
  }
  if (prm.predictor!=1) {
    filter.addFilter(name,prm.getDict(),new PredOutput(new FOutput(filter.getOutput()),prm.width,prm.color,prm.bpc,prm.predictor));
  } else {
    filter.addFilter(name,prm.getDict(),new FOutput(filter.getOutput()));
  }
}
// }}}

// {{{ RLEFilter   - RunLengthDecode
const char *RLEFilter::name="RunLengthDecode";

// {{{ RLEFilter::FInput
RLEFilter::FInput::FInput(Input &read_from) : read_from(read_from),runlen(0),lastchar(0)
{
}

int RLEFilter::FInput::read(char *buf,int len)
{
  int olen=0;
  char c;

  if (runlen==-129) { // EOD
    return 0;
  }
  while (len>0) {
    // read next run
    if (runlen==0) {
      int res=read_from.read(&c,1);
      if (!res) {
        throw UsrError("RLE stream ended without EOD-marker");
      }
      if (c>=0) { // verbatim
        runlen=(int)c+1;
      } else { // repeat / eod
        runlen=(int)c-1;
        if (runlen==-129) { // EOD
          return olen;
        }
        res=read_from.read(&lastchar,1);
        if (!res) {
          throw UsrError("RLE run without character");
        }
      }
    }
    // execute run
    if (runlen>0) { // verbatim
      const int clen=min(len,runlen);
      int res=read_from.read(buf,clen);
      if (res!=clen) {
        throw UsrError("RLE run ended prematurely");
      }
      buf+=res;
      len-=res;
      runlen-=res;
      olen+=res;
    } else { // repeat  (runlen<0 esp. !=0)
      const int clen=min(len,-runlen);
      memset(buf,lastchar,clen*sizeof(char));
      buf+=clen;
      len-=clen;
      runlen+=clen;
      olen+=clen;
    }
  }
  return olen;
}

void RLEFilter::FInput::reset()
{
  runlen=0;
  lastchar=0;
}

long RLEFilter::FInput::pos() const
{
  return -1;
}

void RLEFilter::FInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Reposition in RLEFilter_Input is not supported");
  }
  reset();
  read_from.pos(0);
}
// }}}

// {{{ RLEFilter::FOutput
RLEFilter::FOutput::FOutput(Output &write_to) : write_to(write_to),runlen(0),lastchar(0)
{
}

void RLEFilter::FOutput::write(const char *buf,int len)
{
  if (len<0) {
    len=strlen(buf);
  }
  while (len>0) {
    if (runlen) { // repeat coding
      if (verbbuf.size()) { // write out waiting verbatim stuff
        write_to.put(verbbuf.size()-1);
        write_to.write(&verbbuf[0],verbbuf.size());
        verbbuf.resize(0);
      }
      for (;len>0;buf++,len--,runlen++) {
        if ( (runlen==128)||(lastchar!=*buf) ) { // leave repeat-mode
          write_to.put(-runlen+1);
          write_to.put(lastchar);
          runlen=0;
          break;
        }
      }
    } else { // verbatim coding
      assert(verbbuf.size()<=129);
      if ( (verbbuf.size()==1)&&(verbbuf[0]==*buf) ) { // 2-run
        runlen=2;
      } else if ( (verbbuf.size()>=2)&&(verbbuf.back()==*buf)&&(verbbuf[verbbuf.size()-2]==*buf) ) { // verb + 3-run
        runlen=3;
      } else if ( (verbbuf.size()>=1)&&(len>1)&&(verbbuf.back()==*buf)&&(*buf==buf[1]) ) { // verb + 3-run, 2nd case
        verbbuf.push_back(*buf++);
        len--;
        runlen=3;
      } else if (verbbuf.size()>=128) { // we have no choice
        write_to.put(127);
        write_to.write(&verbbuf[0],128);
        if (verbbuf.size()>128) { // i.e. 129
          verbbuf[0]=verbbuf[128];
        }
        verbbuf.resize(verbbuf.size()-128);
        continue;
      } else if (len==1) {
        verbbuf.push_back(*buf++);
        break; // len--;
      } else { // len>1, verbbuf.size<128, >verbbuf + >buf will not generate a repeat-codeable chunk
        int iA;
        for (iA=1,buf++,len--;len>0;buf++,len--,iA++) { // find 2-run at iA-1 -> (buf[-1],*buf) <- iA
          if (buf[-1]==*buf) {
            break;
          }
        }
        // write out multiples of 128
        if (verbbuf.size()+iA-1>=128) {
          write_to.put(127);
          write_to.write(&verbbuf[0],verbbuf.size());
          write_to.write(buf-iA,128-verbbuf.size());
          iA-=128-verbbuf.size();
          verbbuf.resize(0);
          while (iA-1>=128) {
            write_to.put(127);
            write_to.write(buf-iA,128);
            iA-=128;
          }
        }
        // copy remaining stuff to verbbuf
        verbbuf.resize(verbbuf.size()+iA);
        memcpy(&verbbuf[verbbuf.size()-iA],buf-iA,iA*sizeof(char));
        // now verbbuf <=129
        continue; // will catch 2- or 3-runs
      }
      // start repeat coding
      lastchar=*buf;
      buf++;
      len--;
      verbbuf.resize(verbbuf.size()-runlen+1);
    }
  }
}

void RLEFilter::FOutput::flush()
{
  if (runlen) {
    write_to.put(-runlen+1);
    write_to.put(lastchar);
    runlen=0;
  } else if (verbbuf.size()) { // write out waiting verbatim stuff
    write_to.put(verbbuf.size()-1);
    write_to.write(&verbbuf[0],verbbuf.size());
  }
  write_to.put(-128); // EOD
  lastchar=0;
  verbbuf.clear();
  write_to.flush();
}
// }}}

Input *RLEFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }
  return new FInput(read_from);
}

void RLEFilter::makeOutput(OFilter &filter)
{
  filter.addFilter(name,NULL,new FOutput(filter.getOutput()));
}
// }}}

// {{{ FaxFilter   - CCITTFaxDecode
const char *FaxFilter::name="CCITTFaxDecode";

// {{{ FaxFilter::Params
FaxFilter::Params::Params() : width(1728),kval(0),invert(false),eol(false),eba(false),eob(true),maxdamage(0)
{
}

FaxFilter::Params::Params(const Dict &params) : maxdamage(0)
{
  width=params.getInt_D("Columns",1728);
  kval=params.getInt_D("K",0);
    
//    int rows=params.getInt_D("Rows",0); // dynamic
  eol=params.getBool_D("EndOfLine",false);
  eba=params.getBool_D("EncodeByteAlign",false);
  eob=params.getBool_D("EndOfBlock",true); // overrides Rows...
  invert=params.getBool_D("BlackIs1",false);

  if ( (eol)&&(kval>=0) ) {
    maxdamage=params.getInt_D("DamagedRowsBeforeError",0);
  }
}

Dict *FaxFilter::Params::getDict() const
{
  auto_ptr<Dict> ret(new Dict);

  if (width!=1728) {
    ret->add("Columns",width);
  }
  if (kval!=0) {
    ret->add("K",kval);
  }
  if (eol) {
    ret->add("EndOfLine",eol);
  }
  if (eba) {
    ret->add("EncodeByteAlign",eba);
  }
  if (!eob) {
    ret->add("EndOfBlock",eob);
  }
  if (invert) {
    ret->add("BlackIs1",invert);
  }
  if (maxdamage!=0) {
    ret->add("DamagedRowsBeforeError",0);
  }

  if (ret->empty()) {
    ret.reset();
  }
  return ret.release();
}
// }}}

// NOTE: we have to keep 0 black and 1 white, for compatibility with other pdf filters. 
//   therefore the default bitmap interpretation, which is also used by CCITT cannot be used internally
//   that means, we have to invert on input/output.
//   esp. /BlackIs1 is IMO badly named.
// {{{ FaxFilter::FInput
FaxFilter::FInput::FInput(Input &read_from,int kval,int width,bool invert) : read_from(read_from),invert(invert),eof(false)
{
  outbuf.resize((width+7)/8);
  outpos=outbuf.size();

  state=init_g4_read(kval,width,readfunc_Input,&read_from);
  if (!state) {
    throw bad_alloc();
  }
}

FaxFilter::FInput::~FInput() 
{
  free_g4(state);
}

int FaxFilter::FInput::read(char *buf,int len)
{
  int olen=0;
  const int bwidth=(int)outbuf.size();
  if (outpos!=bwidth) {
    const int clen=min(len,bwidth-outpos);
    memcpy(buf,&outbuf[outpos],clen*sizeof(char));
    outpos+=clen;
    buf+=clen;
    len-=clen;
    olen+=clen;
  }
  if (eof) { // EOF
    return olen;
  }
  while (len>=bwidth) {
    int res=decode_g4(state,(unsigned char *)buf);
    if (res<0) {
      throw UsrError("decode_g4 failed: %d",res);
    }
    if (!invert) { // we need 0=black internally
      for (int iA=0;iA<bwidth;iA++,buf++) {
        *buf^=0xff;
      }
    } else {
      buf+=bwidth;
    }
    if (res==1) { // EOF
      eof=true;
      return olen;
    }
    len-=bwidth;
    olen+=bwidth;
  }
  if (len>0) {
    int res=decode_g4(state,(unsigned char *)&outbuf[0]);
    if (res<0) {
      throw UsrError("decode_g4 failed: %d",res);
    }
    if (!invert) { // we need 0=black internally
      for (int iA=0;iA<bwidth;iA++) {
        outbuf[iA]^=0xff;
      }
    }
    memcpy(buf,&outbuf[0],len*sizeof(char));
    outpos=len;
    olen+=len;
    if (res==1) { // EOF
      eof=true;
      return olen;
    }
  }
  return olen;
}

long FaxFilter::FInput::pos() const
{
  return -1;
}

void FaxFilter::FInput::pos(long pos)
{
  if (pos!=0) {
    throw invalid_argument("Reposition in FaxFilter_Input is not supported");
  }
  outpos=outbuf.size();
  eof=false;
  restart_g4(state);

  read_from.pos(0);
}
// }}}

// {{{  FaxFilter::FOutput
FaxFilter::FOutput::FOutput(Output &write_to,int kval,int width,bool invert) : write_to(write_to),invert(invert)
{
  inbuf.resize((width+7)/8);
  inpos=0;

  state=init_g4_write(kval,width,writefunc_Output,&write_to);
  if (!state) {
    throw bad_alloc();
  }
}

FaxFilter::FOutput::~FOutput()
{
  free_g4(state);
}

void FaxFilter::FOutput::write(const char *buf,int len)
{
  if (len<0) {
    len=strlen(buf);
  }
  const int bwidth=inbuf.size();
  if (inpos>0) {
    const int clen=min(len,bwidth-inpos);
    memcpy(&inbuf[inpos],buf,clen*sizeof(char));
    inpos+=clen;
    if (inpos<bwidth) { // -> len==clen
      return;
    }
    // inpos==bwidth
    buf+=clen;
    len-=clen;
    if (!invert) { // we need 0=black internally
      for (int iA=0;iA<bwidth;iA++) {
        inbuf[iA]^=0xff;
      }
    }
    int res=encode_g4(state,(unsigned char *)&inbuf[0]);
    if (res!=0) {
      throw UsrError("encode_g4 failed: %d",res);
    }
    inpos=0;
  }
  while (len>=bwidth) {
    int res;
    if (!invert) { // we need 0=black internally
      for (int iA=0;iA<bwidth;iA++) {
        inbuf[iA]=buf[iA]^0xff;
      }
      res=encode_g4(state,(unsigned char *)&inbuf[0]);
    } else {
      res=encode_g4(state,(unsigned char *)buf);
    }
    if (res!=0) {
      throw UsrError("encode_g4 failed: %d",res);
    }
    buf+=bwidth;
    len-=bwidth;
  }
  if (len>0) {
    memcpy(&inbuf[0],buf,len*sizeof(char));
    inpos+=len;
  }
}

void FaxFilter::FOutput::flush()
{
  int res=encode_g4(state,NULL);
  if (res!=0) {
    throw UsrError("encode_g4 failed: %d",res);
  }
  inpos=0;
  restart_g4(state);
  write_to.flush();
}
// }}}

Input *FaxFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }

  if (params) {
    Params prm(*params);

    if ( (prm.eol)||(prm.eba)||(!prm.eob) ) {
      fprintf(stderr,"WARNING: Unsupported CCITT mode:\n");
      dump(params);
    }
    return new FInput(read_from,prm.kval,prm.width,prm.invert);
  } else {
    return new FInput(read_from,0,1728,false);
  }
}

void FaxFilter::makeOutput(OFilter &filter,const Params &prm)
{
  if ( (prm.eol)||(prm.eba)||(!prm.eob) ) {
    throw UsrError("Unsupported CCITT mode");
  }
  filter.addFilter(name,prm.getDict(),new FOutput(filter.getOutput(),prm.kval,prm.width,prm.invert));
}
// }}}

// TODO: 
// {{{ JBIG2Filter - JBIG2Decode
const char *JBIG2Filter::name="JBIG2Decode";

Input *JBIG2Filter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }
  return NULL; // not implemented
}

void JBIG2Filter::makeOutput(OFilter &filter,const Params &prm)
{
  fprintf(stderr,"WARNING: Output filter /%s not implemented\n",name);
//  filter.addFilter(name,prm->getDict(),new FOutput(filter.getOutput(),prm.kval,prm.width,prm.invert));
}
// }}}

// {{{ JpegFilter  - DCTDecode
const char *JpegFilter::name="DCTDecode";

// {{{ JpegFilter::Params
JpegFilter::Params::Params() : quality(75),width(0),height(0),color(1),colortransform(-1)
{
}

JpegFilter::Params::Params(const Dict &params) : quality(75),width(0),height(0),color(1)
{
  colortransform=params.getInt_D("ColorTransform",-1);
}

Dict *JpegFilter::Params::getDict() const
{
  if (colortransform==-1) {
    return NULL;
  }
  auto_ptr<Dict> ret(new Dict);

  ret->add("ColorTransform",colortransform);

  return ret.release();
}
// }}}

struct ourj_errmgr
{
  struct jpeg_error_mgr super;
  jmp_buf jb;
  char msg[JMSG_LENGTH_MAX];
};

struct ourj_srcmgr
{
  struct jpeg_source_mgr super;
  Input *in;
  JOCTET *buf;
};

struct ourj_dstmgr
{
  struct jpeg_destination_mgr super;
  Output *out;
  JOCTET *buf;
};

// {{{ init_src(cinfo,in)/init_dst(cinfo,out); ourj_{sd}... Jpeg src/dst, ourj_eexit
extern "C" {
#define BUFSIZE 4096
  static void ourj_eexit(j_common_ptr cinfo) {
    ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
    oer->super.format_message(cinfo,oer->msg);
    longjmp(oer->jb, 1);
  }

  static void ourj_sinit(j_decompress_ptr cinfo) { }
  static void ourj_sterm(j_decompress_ptr cinfo) { }
  static boolean ourj_sfill(j_decompress_ptr cinfo) {
    ourj_srcmgr *osm=(ourj_srcmgr *)cinfo->src;

    Input *in=(Input *)osm->in;
    int ret=in->read((char *)osm->buf,BUFSIZE);

    if (ret<=0) {
      ERREXIT(cinfo,JERR_INPUT_EMPTY);
    }
    osm->super.next_input_byte=osm->buf;
    osm->super.bytes_in_buffer=ret;
    return TRUE;
  }
  static void ourj_sskip(j_decompress_ptr cinfo,long num_bytes) {
    ourj_srcmgr *osm=(ourj_srcmgr *)cinfo->src;

    if (num_bytes > 0) {
      while (num_bytes>(long)osm->super.bytes_in_buffer) {
        num_bytes-=(long)osm->super.bytes_in_buffer;
        (void)ourj_sfill(cinfo);
      }
      osm->super.next_input_byte+=num_bytes;
      osm->super.bytes_in_buffer-=num_bytes;
    }
  }

  static void ourj_dinit(j_compress_ptr cinfo) {
    ourj_dstmgr *odm=(ourj_dstmgr *)cinfo->dest;

    odm->super.next_output_byte=odm->buf;
    odm->super.free_in_buffer=BUFSIZE;
  }
  static void ourj_dterm(j_compress_ptr cinfo) {
    ourj_dstmgr *odm=(ourj_dstmgr *)cinfo->dest;

    Output *out=(Output *)odm->out;

    // remaining data?
    const int len=BUFSIZE-odm->super.free_in_buffer;
    if (len>0) {
      out->write((char *)odm->buf,len);
    }

    out->flush();
  }
  static boolean ourj_dempty(j_compress_ptr cinfo) {
    ourj_dstmgr *odm=(ourj_dstmgr *)cinfo->dest;

    Output *out=(Output *)odm->out;

    out->write((char *)odm->buf,BUFSIZE);

    odm->super.next_output_byte=odm->buf;
    odm->super.free_in_buffer=BUFSIZE;

    return TRUE;
  }
};

static void init_src(j_decompress_ptr cinfo,Input &read_from)
{
  assert(cinfo);

  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error: %s",oer->msg);
  }

  cinfo->src=(struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)
                 ((j_common_ptr)cinfo,JPOOL_PERMANENT,sizeof(ourj_srcmgr));

  ourj_srcmgr *osm=(ourj_srcmgr *)cinfo->src;

  osm->buf=(JOCTET *)(*cinfo->mem->alloc_small)
                 ((j_common_ptr)cinfo,JPOOL_PERMANENT,BUFSIZE*sizeof(JOCTET));

  osm->super.init_source=ourj_sinit;
  osm->super.fill_input_buffer=ourj_sfill;
  osm->super.skip_input_data=ourj_sskip;
  osm->super.resync_to_restart=jpeg_resync_to_restart;
  osm->super.term_source=ourj_sterm;
  osm->super.bytes_in_buffer=0;
  osm->super.next_input_byte=NULL;
  osm->in=&read_from;
}

static void init_dst(j_compress_ptr cinfo,Output &write_to)
{
  assert(cinfo);

  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error: %s",oer->msg);
  }

  cinfo->dest=(struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)
                 ((j_common_ptr)cinfo,JPOOL_PERMANENT,sizeof(ourj_dstmgr));

  ourj_dstmgr *odm=(ourj_dstmgr *)cinfo->dest;

  odm->buf=(JOCTET *)(*cinfo->mem->alloc_small)
                 ((j_common_ptr)cinfo,JPOOL_PERMANENT,BUFSIZE*sizeof(JOCTET));

  odm->super.init_destination=ourj_dinit;
  odm->super.empty_output_buffer=ourj_dempty;
  odm->super.term_destination=ourj_dterm;
  odm->super.next_output_byte=NULL;
  odm->super.free_in_buffer=0;

  odm->out=&write_to;
}
#undef BUFSIZE
// }}}

// {{{ JpegFilter::FInput
JpegFilter::FInput::FInput(Input &read_from,int colortransform) : read_from(read_from),colortransform(colortransform)
{
  if ( (colortransform>0)&&(colortransform!=1) )  {
    throw UsrError("Unknown /ColorTransform: %d",colortransform);
  }
  cinfo=new jpeg_decompress_struct;
  cinfo->err=NULL;

  try {
    ourj_errmgr *oer=new ourj_errmgr;
    cinfo->err=jpeg_std_error(&oer->super);
    cinfo->err->error_exit=ourj_eexit;

    if (setjmp(oer->jb)) {
      throw UsrError("jpeg error (Input::ctor): %s",oer->msg);
    }

    jpeg_create_decompress(cinfo);
    init_src(cinfo,read_from);
    // setjmp is redirected by init_src
  } catch (...) {
    delete cinfo->err;
    jpeg_destroy_decompress(cinfo); 
    delete cinfo; 
    throw; 
  }
}

JpegFilter::FInput::~FInput() 
{
  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error (Input::dtor): %s",oer->msg);
  }

  delete cinfo->err;
  jpeg_destroy_decompress(cinfo);
  delete cinfo;
}

int JpegFilter::FInput::read(char *buf,int len)
{
  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error (Input::read): %s",oer->msg);
  }

  if (outbuf.empty()) { // header not yet read
    int res=jpeg_read_header(cinfo,TRUE);
    if (res!=JPEG_HEADER_OK) {
      throw UsrError("jpeg_read_header failed");
    }
    // color=cinfo->num_components

    if (!cinfo->saw_Adobe_marker) {
      if (cinfo->num_components==3) {
        if (colortransform==0) {
          cinfo->jpeg_color_space=JCS_RGB;
        } else if (colortransform==1) {
          cinfo->jpeg_color_space=JCS_YCbCr;
        }
      } else if (cinfo->num_components==4) {
        if (colortransform==0) {
          cinfo->jpeg_color_space=JCS_CMYK;
        } else if (colortransform==1) {
          cinfo->jpeg_color_space=JCS_YCCK;
        }
      }
    }
    // color=cinfo->output_components

    boolean r2=jpeg_start_decompress(cinfo);
    assert(r2==TRUE);

    // width=cinfo->output_width
    // height=cinfo->output_heigth

    outbuf.resize(cinfo->output_width*cinfo->num_components);
    outpos=outbuf.size();
  }
  if (len==0) {
    return 0;
  }
  JSAMPROW row_pointer[1];

  int olen=0;
  const int bwidth=(int)outbuf.size();
  if (outpos!=bwidth) {
    const int clen=min(len,bwidth-outpos);
    memcpy(buf,&outbuf[outpos],clen*sizeof(char));
    outpos+=clen;
    buf+=clen;
    len-=clen;
    olen+=clen;
  }
  if (cinfo->output_scanline==cinfo->output_height) { // EOF
    return olen;
  }
  while (len>=bwidth) {
    row_pointer[0]=(JSAMPLE *)buf;
    int res=jpeg_read_scanlines(cinfo,row_pointer,1);
    if (res!=1) {
      throw UsrError("jpeg scanline not read");
    }
    buf+=bwidth;
    len-=bwidth;
    olen+=bwidth;
    if (cinfo->output_scanline==cinfo->output_height) { // EOF
      jpeg_finish_decompress(cinfo);
      return olen;
    }
  }
  if (len>0) {
    row_pointer[0]=(JSAMPLE *)&outbuf[0];
    int res=jpeg_read_scanlines(cinfo,row_pointer,1);
    if (res!=1) {
      throw UsrError("jpeg scanline not read");
    }
    memcpy(buf,&outbuf[0],len*sizeof(char));
    outpos=len;
    olen+=len;
    if (cinfo->output_scanline==cinfo->output_height) { // EOF
      jpeg_finish_decompress(cinfo);
    }
  }
  return olen;
}

void JpegFilter::FInput::get_params(int &width,int &height,int &color)
{
  if (outbuf.empty()) { // header not yet read
    read(NULL,0);
  }
  width=cinfo->output_width;
  height=cinfo->output_height;
  color=cinfo->num_components;
}

long JpegFilter::FInput::pos() const
{
  return -1;
}

void JpegFilter::FInput::pos(long pos)
{
  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error (Input::pos): %s",oer->msg);
  }

  if (pos!=0) {
    throw invalid_argument("Reposition in JpegFilter_Input is not supported");
  }
  if (!outbuf.empty()) { // header read
    if (cinfo->output_scanline==cinfo->output_height) { // EOF
      jpeg_finish_decompress(cinfo);
    } else {
      jpeg_abort_decompress(cinfo); // TODO: is this allowed, after all?
    }
  }
  outbuf.resize(0); // header not yet read

  read_from.pos(0);
}
// }}}

// {{{  JpegFilter::FOutput
JpegFilter::FOutput::FOutput(Output &write_to,int quality,int width,int height,int color,int colortransform) : write_to(write_to),width(width),color(color)
{
  cinfo=new jpeg_compress_struct;
  cinfo->err=NULL;
    
  try {
    ourj_errmgr *oer=new ourj_errmgr;
    cinfo->err=jpeg_std_error(&oer->super);
    cinfo->err->error_exit=ourj_eexit;

    if (setjmp(oer->jb)) {
      throw UsrError("jpeg error: %s",oer->msg);
    }

    jpeg_create_compress(cinfo);
    init_dst(cinfo,write_to);

    // init_dst has restarted
    if (setjmp(oer->jb)) {
      throw UsrError("jpeg error: %s",oer->msg);
    }

    cinfo->image_width=width;
    cinfo->image_height=height;
    cinfo->input_components=color;
   
    if (color==1) {
      cinfo->in_color_space=JCS_GRAYSCALE;
    } else if (color==3) {
      cinfo->in_color_space=JCS_RGB;
    } else if (color==4) {
      cinfo->in_color_space=JCS_CMYK;
    } else {
      cinfo->in_color_space=JCS_UNKNOWN;
    }
    jpeg_set_defaults(cinfo);
    jpeg_set_quality(cinfo,quality,FALSE); // TRUE in pdf<1.3 ?? TODO?

    if ( (color==3)&&(colortransform==0) ) { // default: 1 (libjpeg: YCbCr)
      jpeg_set_colorspace(cinfo,JCS_RGB);
    } else if ( (color==4)&&(colortransform==1) ) { // default: 0 (libjpeg: CMYK)
      jpeg_set_colorspace(cinfo,JCS_YCCK);
    }
    // TODO? write Adobe (not JFIF marker) when c==3 and colortransform==1
    // cinfo->write_JFIF_header=FALSE; cinfo->write_Adobe_marker==TRUE;

    inbuf.resize(width*color);
    inpos=-1; // no header yet
  } catch (...) {
    delete cinfo->err;
    jpeg_destroy_compress(cinfo); 
    delete cinfo; 
    throw; 
  }
}

JpegFilter::FOutput::~FOutput()
{
  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error: %s",oer->msg);
  }

  delete cinfo->err;
  jpeg_destroy_compress(cinfo);
  delete cinfo;
}

void JpegFilter::FOutput::write(const char *buf,int len)
{
  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error: %s",oer->msg);
  }

  if (len<0) {
    len=strlen(buf);
  }
  JSAMPROW row_pointer[1];

  if (inpos==-1) { // write header
    jpeg_start_compress(cinfo,TRUE);
    inpos=0; 
  }
  const int bwidth=width*color;
  if (inpos>0) {
    const int clen=min(len,bwidth-inpos);
    memcpy(&inbuf[inpos],buf,clen*sizeof(char));
    inpos+=clen;
    if (inpos<bwidth) { // -> len==clen
      return;
    }
    // inpos==bwidth
    buf+=clen;
    len-=clen;
    row_pointer[0]=(JSAMPLE *)&inbuf[0];
    int res=jpeg_write_scanlines(cinfo,row_pointer,1);
    if (res!=1) {
      throw UsrError("jpeg scanline not written");
    }
    inpos=0;
  }
  while (len>=bwidth) {
    row_pointer[0]=(JSAMPLE *)buf;
    int res=jpeg_write_scanlines(cinfo,row_pointer,1);
    if (res!=1) {
      throw UsrError("jpeg scanline not written");
    }
    buf+=bwidth;
    len-=bwidth;
  }
  if (len>0) {
    memcpy(&inbuf[0],buf,len*sizeof(char));
    inpos+=len;
  }
}

void JpegFilter::FOutput::flush()
{
  ourj_errmgr *oer=(ourj_errmgr *)cinfo->err;
  if (setjmp(oer->jb)) {
    throw UsrError("jpeg error: %s",oer->msg);
  }

  // TODO? check number of written lines?
  jpeg_finish_compress(cinfo);
  inpos=-1;

  write_to.flush();
  // TODO: restart not tested
}
// }}}

Input *JpegFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }

  if (params) {
    Params prm(*params);

    return new FInput(read_from,prm.colortransform);
  } else {
    return new FInput(read_from);
  }
}

void JpegFilter::makeOutput(OFilter &filter,const Params &prm)
{
  filter.addFilter(name,prm.getDict(),new FOutput(filter.getOutput(),prm.quality,prm.width,prm.height,prm.color,prm.colortransform));
}
// }}}

// {{{ JPXFilter   - JPXDecode
const char *JPXFilter::name="JPXDecode";

Input *JPXFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }
  return NULL; // not implemented
}

void JPXFilter::makeOutput(OFilter &filter)
{
  fprintf(stderr,"WARNING: Output filter /%s not implemented\n",name);
//  filter.addFilter(name,NULL,new FOutput(filter.getOutput()));
}
// }}}

// {{{ CryptFilter - Crypt
const char *CryptFilter::name="Crypt";

Input *CryptFilter::makeInput(const char *fname,Input &read_from,const Dict *params)
{
  if (strcmp(fname,name)!=0) {
    return NULL;
  }
  fprintf(stderr,"WARNING: Crypt not implemented\n");
  return NULL; // not implemented
}

void CryptFilter::makeOutput(OFilter &filter,const Params &prm)
{
  fprintf(stderr,"WARNING: Output filter /%s not implemented\n",name);
//  filter.addFilter(name,prm->getDict(),new FOutput(filter.getOutput(),prm.kval,prm.width,prm.invert));
}
// }}}
