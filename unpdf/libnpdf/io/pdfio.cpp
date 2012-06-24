#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "pdfio.h"

#include "../parse/ctype.h"

#include "../parse/pdfparse.h" // TODO skip_eol

#include "exception.h"
//#include "pdfcrypt.h"
//#include "filesystem.h"
//#include "tools.h"

#include "../objs/all.h" // TODO fixes other includes
#include "sub.h" // TODO fixes other includes

using namespace PDFTools;
using namespace std;

// FIXME: rework as tokenizer
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

// TODO FIXME: this is a classical tokenizer
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
    prepos=start;
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

