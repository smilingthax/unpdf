#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "exception.h"
#include "pdfparse.h"

#include "ctype.h"

#include "../io/mem.h"
#include "../io/sub.h"
#include "../objs/all.h"

#include "../stream/pdfcomp.h" // FIXME
#include "../io/pdfio.h" // FIXME
#include "../pdf/pdf.h" // FIXME

using namespace std;
using namespace PDFTools;

Object *PDFTools::Parser::parse(const char *string) // {{{
{
  MemInput mn(string,strlen(string));
  ParsingInput pi(mn);

  return parse(pi);
}
// }}}

Object *PDFTools::Parser::parse(ParsingInput &in,const Decrypt *str_decrypt) // {{{
{
  in.skip(false);

  char buf[2];
  int res=in.read(buf,2);
  if (res==0) {
    return NULL;
  }
  in.unread(buf,res); // data also stays in >buf !

  if (*buf=='(') {
    return parseString(in,str_decrypt);
  } else if (*buf=='<') {
    if ( (res==2)&&(buf[1]=='<') ) {
      return parseDict(in,str_decrypt);
    }
    return parseHexstring(in,str_decrypt);
  } else if (*buf=='[') {
    return parseArray(in,str_decrypt);
  } else if (*buf=='{') {
    throw UsrError("Illegal '{'");
  } else if (*buf=='/') {
    return parseName(in);
  } else if ( (isdigit(*buf))||(*buf=='.')||(*buf=='-') ) {
    return parseNum(in);
  } else if ( (*buf==')')||(*buf=='>')||(*buf==']')||(*buf=='}') ) {
    throw UsrError("Out of place terminator '%c'",*buf);
  }
  if (in.next("true")) {
    return new Boolean(true);
  } else if (in.next("false")) {
    return new Boolean(false);
  } else if (in.next("null")) {
    return new Object(); // null-object
  } 
  throw UsrError("Unknown entity: \"%.2s\"",buf);
}
// }}}

Object *PDFTools::Parser::parseObj(PDF *pdf,SubInput &in,const Ref *ref) // {{{
{
  long startpos=in.basepos(); // only for parse errors; must be called on SubInput
 
  auto_ptr<Decrypt> str_decrypt;
  if ( (pdf)&&(ref) ) {
    str_decrypt.reset(pdf->getStrDecrypt(*ref));
  }

  ParsingInput psi(in);
  parseObjNum(psi,startpos,ref);

  auto_ptr<Object> ret(Parser::parse(psi,str_decrypt.get()));
  if (!ret.get()) { // no more input
    return NULL;
  }
  Dict *dictval=dynamic_cast<Dict *>(ret.get());
  long streamstart=-1,streamend=-1;
  if (dictval) { // check for stream
    psi.skip(false);
    if (psi.next("stream")) {
      if ( (!psi.next("\r\n"))&&(!psi.next('\n')) ) {
        throw UsrError("Linebreak expected after \"stream\"");
      }

      streamstart=psi.pos();

      int length;
      if (!pdf) { // i.e. must be direct
        const Object *obj=dictval->find("Length");
        const NumInteger *ival=dynamic_cast<const NumInteger *>(obj);
        if (!ival) {
          throw UsrError("/%s is not a direct Integer","Length");
        }
        length=ival->value();
      } else {
        // this might reposition
        length=dictval->getInt(*pdf,"Length");
      }
  if (dictval->find("F")) {
    printf("WARNING: external stream file not yet supported\n");
  }

      streamend=streamstart+length;
      psi.pos(streamend);
      
      // skip eol
      char buf[2];
      int res=psi.read(buf,2);
      if (res<2) {
        throw UsrError("Stream not properly terminated at %ld",streamend);
      }
      res=Parser::skip_eol(buf);
      psi.unread(buf+res,2-res);

      if (!psi.next("endstream")) {
        throw UsrError("\"endstream\" expected at %ld",streamend);
      }
    }
  }
  psi.skip(false);
  if (!psi.next("endobj")) {
    if (ref) {
      throw UsrError("Object %d %d R not properly terminated at %ld",ref->ref,ref->gen,in.basepos()+psi.pos()-in.pos());
    } else {
      throw UsrError("Object not properly terminated at %ld",in.basepos()+psi.pos()-in.pos());
    }
  }

  if ( (dictval)&&(streamstart!=-1) ) { // stream part 2
// TODO  pdf->getEFFDecrypt() ... /EmbeddedFile... if (*ref) ...
//  ret.reset(new InStream(pdf,dictval,new SubInput(in,streamstart,streamend),ref.release(),pdf->getEffDecrypt(*ref)));
    // Fortunately the only real user is pdf.getObject
    ret.reset(new InStream(pdf,dictval,new SubInput(in,streamstart,streamend),ref));
  }

  return ret.release();
}
// }}}

// Note: we /could/ use resno and resgen for decryption, if ref not given ...
void PDFTools::Parser::parseObjNum(ParsingInput &in,long startpos,const Ref *ref) // {{{
{
  try {
    int resno=in.readUInt();
    in.skip(true);
    int resgen=in.readUInt();
    in.skip(true);
    if (!in.next("obj")) {
      throw UsrError("Not a valid object");
    }
    if (  (ref)&&( (resno!=ref->ref)||(resgen!=ref->gen) )  ) {
      throw UsrError("Corrupt xref for object");
    }
  } catch (UsrError &uex) {
    if (ref) {
      throw UsrError("%s at %ld for %d %d R",uex.what(),startpos,ref->ref,ref->gen);
    } else {
      throw UsrError("%s at %ld",uex.what(),startpos);
    }
  }
}
// }}}

Object *PDFTools::Parser::parseNum(ParsingInput &in) // {{{  may return: NumFloat NumInt Ref
{
  int res=in.readInt();

  if (in.next('.')) {
    return new NumFloat(in.readFrac(res));
  }
  if (res<0) {
    return new NumInteger(res);
  }
  // test if this is a reference...
  in.skip(false);
  pair<const char *,int> rs=in.pread_to_delim(true);
  const char *buf=rs.first;
  int len=rs.second;
  int iA=0;
  for (;iA<len;iA++) {
    if (!isdigit(buf[iA])) {
      break;
    }
  }
  if ( (iA!=0)&&(iA<6)&&(Parser::is_space(buf[iA])) ) {
    // TODO: skip arbitrary number of space
    for (iA++;iA<len;iA++) {
      if (!Parser::is_space(buf[iA])) {
        break;
      }
    }
    if (buf[iA]=='R') {
      // ok we got one
      in.unread(buf+iA+1,len-iA-1);
      int gen=0;
      for (iA=0;buf[iA]>='0';iA++) { 
        gen*=10;
        gen+=buf[iA]-'0';
      }
      return new Ref(res,gen);
    }
  }
  // only a simple number
  in.unread(buf,len);
  in.unread(" ",1); // undo the skip()
  return new NumInteger(res);
}
// }}}

String *PDFTools::Parser::parseString(ParsingInput &in,const Decrypt *str_decrypt) // {{{
{
  if (!in.next('(',41)) {
    throw UsrError("Not a String");
  }
  std::string ret;
  int pos=0;
  int paren=1;
  while (paren>0) {
    ret.resize(pos+40,0);
    int res=in.read(&ret[pos],40);
    if (!res) {
      throw UsrError("String not properly terminated");
    }
    ret.resize(pos+res);
    for (;pos<(int)ret.size();pos++) {
      const int r=Parser::skip_eol(&ret[pos]);
      if (r) { // treated as single \n
        ret[pos]='\n';
        if (r>1) {
          memmove(&ret[pos+1],&ret[pos+r],ret.size()-pos-r);
          ret.resize(ret.size()-r+1);
        }
      } else if (ret[pos]=='\\') {
        in.unread(&ret[pos],ret.size()-pos);
        int c=in.read_escape();
        if (c==-1) {
          throw UsrError("String ended prematurely");
        } else if (c==-2) { // empty substitution
          ret.resize(pos);
        } else {
          ret[pos]=c;
          ret.resize(pos+1);
        }
      } else if (ret[pos]==')') {
        if (!--paren) {
          break;
        }
      } else if (ret[pos]=='(') {
        paren++;
      }
    }
  }
  in.unread(&ret[pos+1],ret.size()-pos-1); // excluding paren
  ret.resize(pos);
  return new String(ret,str_decrypt);
}
// }}}

String *PDFTools::Parser::parseHexstring(ParsingInput &in,const Decrypt *str_decrypt) // {{{
{
  if (!in.next('<',40)) {
    throw UsrError("Not a Hexstring");
  }
  std::string ret;
  int readpos=0,writepos=0;
  bool second=false;
  while (1) {
    ret.resize(readpos+40,0);
    int res=in.read(&ret[readpos],40);
    if (!res) {
      throw UsrError("Hexstring not properly terminated");
    }
    ret.resize(readpos+res);
    for (;readpos<(int)ret.size();readpos++) {
      if (isxdigit(ret[readpos])) {
        if (second) {
          ret[writepos]+=Parser::one_hex(ret[readpos]);
          writepos++;
        } else {
          ret[writepos]=Parser::one_hex(ret[readpos])<<4;
        }
        second=!second;
      } else if (ret[readpos]=='>') { // done
        readpos++;
        in.unread(&ret[readpos],ret.size()-readpos);
        ret.resize(writepos+second);
        return new String(ret,str_decrypt,true);
      } else if (!Parser::is_space(ret[readpos])) {
        throw UsrError("Bad Hexstring");
      }
    }
    readpos=writepos+second;
  }
  assert(0); // Not reached
}
// }}}

Name *PDFTools::Parser::parseName(ParsingInput &in) // {{{
{
  if (!in.next("/")) {
    throw UsrError("Not a name");
  }
  pair<const char *,int> res=in.pread_to_delim();
  if (!res.second) {
    throw UsrError("Empty name string");
  }
  // calc size
  int len=0;
  for (int iA=0;iA<res.second;iA++,len++) {
    if (res.first[iA]=='#') {
      if ( (!isxdigit(res.first[iA+1]))||
           (!isxdigit(res.first[iA+2]))||
           (iA+2>=res.second) ) {
        throw UsrError("Bad escape sequence in name");
      }
      iA+=2;
    }
  }
  // unescape
  char *ret=(char *)malloc((len+1)*sizeof(char));
  if (!ret) {
    throw bad_alloc();
  }
  for (int iA=0,len=0;iA<res.second;iA++,len++) {
    if (res.first[iA]=='#') {
      ret[len]=(Parser::one_hex(res.first[iA+1])<<4)+Parser::one_hex(res.first[iA+2]);
      iA+=2;
    } else {
      ret[len]=res.first[iA];
    }
  }
  ret[len]=0;

  return new Name(ret,Name::TAKE);
}
// }}}

Array *PDFTools::Parser::parseArray(ParsingInput &in,const Decrypt *str_decrypt) // {{{
{
  if (!in.next('[',40)) {
    throw UsrError("Not an Array");
  }
  auto_ptr<Array> ret(new Array);
  in.skip(false);
  while (!in.next(']')) {
    const Object *obj=parse(in,str_decrypt);
    if (!obj) {
      throw UsrError("Array not properly terminated");
    }
    ret->add(obj,true);
    in.skip(false);
  }
  return ret.release();
}
// }}}

Dict *PDFTools::Parser::parseDict(ParsingInput &in,const Decrypt *str_decrypt) // {{{
{
  if (!in.next("<<",80)) {
    throw UsrError("Not a Dictionary");
  }
  auto_ptr<Dict> ret(new Dict);
  in.skip(false);
  while (!in.next(">>")) {
    auto_ptr<Name> key(parseName(in));
    if (!key.get()) {
      throw UsrError("Dictionary not properly terminated");
    }
    in.skip(false);
    Object *obj=parse(in,str_decrypt);
    if (!obj) {
      throw UsrError("Dictionary: key without value");
    }
    ret->add(key->value(),obj,true);
    in.skip(false);
  }
  return ret.release();
}
// }}}

pair<int,long> PDFTools::Parser::read_pdf(Input &fi) // {{{
{
  char buf[1025];
  int rlen=fi.read(buf,1024);
  buf[rlen]=0;

  // TODO? also check for %PS-Adobe-N.n PDF-M.m  (only impl note for acro)
  char *tmp=strstr(buf,"%PDF-");
  if ( (!tmp)||(rlen-(tmp-buf)<8)||(!isdigit(tmp[5]))||(tmp[6]!='.')||(!isdigit(tmp[7])) ) {
    throw UsrError("No pdf version");
  }
  int version=(tmp[5]-'0')*10+(tmp[7]-'0');

  if (rlen==1024) { // i.e. pdf is not smaller than 1k, otherwise the pos() will fail.
    fi.pos(-1024);
    rlen=fi.read(buf,1024);
    buf[rlen]=0;
  }

  for (tmp=buf+rlen-1-9;tmp>=buf;tmp--) {
    if (strncmp(tmp,"startxref",9)==0) {
      break;
    }
  }
  if (tmp<buf) {
    throw UsrError("startxref not found");
  }
  tmp+=9;
  tmp+=strspn(tmp,"\n\r");
  unsigned long xrefpos;
  char *t2;
  xrefpos=strtoul(tmp,&t2,10);
  t2+=strspn(t2,"\n\r");
  if (strncmp(t2,"%%EOF",5)!=0) {
    throw UsrError("EOF not found");
  }
  assert(xrefpos<=LONG_MAX);
  
  return make_pair(version,xrefpos);
}
// }}}

auto_ptr<PDF> PDFTools::open_pdf(Input &fi) // {{{
{
  pair<int,long> res=Parser::read_pdf(fi);
  
  return auto_ptr<PDF>(new PDF(fi,res.first,res.second));
}
// }}}

