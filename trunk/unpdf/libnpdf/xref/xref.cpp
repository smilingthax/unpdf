#include "xref.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include "exception.h"
#include "../objs/all.h"
#include "../io/sub.h"
#include "../io/ptr.h"

#include "../io/pdfio.h"  // FIXME
#include "../parse/pdfparse.h"  // FIXME
#include "../stream/pdfcomp.h"  // FIXME

#include "../io/file.h"

namespace PDFTools {

  extern FILEOutput stdfo;

XRef::XRef(bool writeable) // {{{
{
  if (writeable) {
    // xrefpos.empty();
    // first empty entry
    xref.push_back(xre_t());
    xref.front().gen=65535;
  } else {
    xrefpos.push_back(-1);
  }
}
// }}}

Ref XRef::newRef(long pos) // {{{
{
  assert(xrefpos.empty()); // create-mode
  Ref ret(xref.size());
  xref.push_back(xre_t(pos));
  return ret;
}
// }}}

void XRef::setRef(const Ref &ref,long pos) // {{{
{
  assert(xrefpos.empty()); // create-mode
  if ( (ref.ref<0)||(ref.ref>=(int)xref.size()) ) {
    throw UsrError("Bad reference");
  }
  if ( (xref[ref.ref].type!=xre_t::XREF_USED)||(xref[ref.ref].off!=-1) ) {
    throw UsrError("Re-setting xref is not supported");
  }
  xref[ref.ref].off=pos;
  xref[ref.ref].gen=ref.gen; // TODO?
}
// }}}

void XRef::clear() // {{{
{
  assert(xrefpos.empty()); // create-mode
  xref.clear();
}
// }}}

bool XRef::parse(ParsingInput &pi,ParseMode mode) // {{{
{
  assert(!xrefpos.empty()); // non-create-mode
  long xrp=pi.pos();

  bool ret=false;
  if (mode==BOTH) {
    ret=read_xref(pi,xref,false);
  } else if (mode==AS_STREAM) {
    ret=read_xref_stream(pi,xref);
  } else if (mode==AS_TABLE) {
    ret=read_xref(pi,xref,true);
  }
  if (ret) {
    // we need to store this xrefs position to generate accurate ends
    if (xrefpos[0]==-1) {
      xrefpos[0]=xrp;
    } else {
      xrefpos.push_back(xrp);
    }
  }
  generate_ends(); // TODO? optimize/ only once
  return ret;
}
// }}}

bool XRef::read_xref(ParsingInput &fi,XRefVec &to,bool ignore_stream) // {{{
{
  if (!fi.next("xref")) {
    if (!ignore_stream) {
      // TODO?  reposition  ... done implictly there.
      return read_xref_stream(fi,to);
    }
    return false;
  }
  fi.skip();
  char c; 
  do {
    int offset=fi.readUInt();
    fi.skip();
    int len=fi.readUInt();
    fi.skip();
    for (int iA=0;iA<len;iA++) {
      // "%010u %05d %c[\r\n][\r\n]"
      char buf[20];
      if (fi.read(buf,20)!=20) {
        return false;
      }
      xre_t xr;
      xr.off=readUIntOnly(buf,10);
      xr.gen=readUIntOnly(buf+11,5);
      if (buf[17]=='f') {
        xr.type=xre_t::XREF_FREE;
      } else if (buf[17]=='n') {
        xr.type=xre_t::XREF_USED;
      } else {
        return false;
      }
      if (offset+iA>=(int)to.size()) {
        to.resize(offset+iA+1,xre_t());
      }
      if (to[offset+iA].gen<xr.gen) {
        to[offset+iA]=xr;
      }
    }
    int res=fi.read(&c,1);
    if (res==0) {
      return false;
    }
    fi.unread(&c,1);
  } while (isdigit(c));
  return true;
}
// }}}

// TODO?! don't create SubInput on top of ParsingInput...
bool XRef::read_xref_stream(ParsingInput &fi,XRefVec &to) // {{ {
{
  SubInput si(fi,fi.pos(),-1);  // don't know the end...; this repositions.
  // pdf==NULL: this requires /Filter and /DecodeParms to be direct.
  // FIXME: this also reqires /Length to be direct -- but spec doesn't guarantee!
  std::auto_ptr<Object> obj(Parser::parseObj(NULL,si,NULL));

  InStream *stmval=dynamic_cast<InStream *>(obj.get());
  if (!stmval) {
    return false;
  }

  const Dict &dict=stmval->getDict();
  // the following entries must be direct.

  const Name *nval=dynamic_cast<const Name *>(dict.find("Type"));
  if ( (!nval)||(strcmp(nval->value(),"XRef")!=0) ) {
    throw UsrError("/Type not /XRef, as required");
  }

  const Array *wval=dynamic_cast<const Array *>(dict.find("W"));
  if (!wval) {
    throw UsrError("/W not an Array");
  } else if (wval->size()!=3) {
    throw UsrError("/W must have length 3");
  }
  int flens[3],flen=0;
  for (int iA=0;iA<3;iA++) {
    const NumInteger *ival=dynamic_cast<const NumInteger *>((*wval)[iA]);
    if (!ival) {
      throw UsrError("One of the /W entries is not an Integer");
    }
    flens[iA]=ival->value();
    if ( (flens[iA]<0)||(flens[iA]>8) ) {
      throw UsrError("Bad /W entry");
    }
    flen+=flens[iA];
  }

  const NumInteger *ival=dynamic_cast<const NumInteger *>(dict.find("Size"));
  if (!ival) {
    throw UsrError("/Size not an Integer");
  }

  int maxlen=ival->value();

  Array adef;
  const Array *aval=dynamic_cast<const Array *>(dict.find("Index"));
  if (!aval) {
    if (dict.find("Index")) {
      throw UsrError("/Index has wrong type (not Array)");
    }
    adef.add(new NumInteger(0),true);
    adef.add(new NumInteger(maxlen),true);
    aval=&adef;
  }

  const int alen=aval->size();
  if (alen%2!=0) {
    throw UsrError("/Index has bad size");
  }

  InputPtr in=stmval->open();
  char buf[3*8];
  for (int iA=0;iA<alen;iA+=2) {
    int offset=aval->getUInt_D(iA);
    int len=aval->getUInt_D(iA+1);
    
    for (int iB=0;iB<len;iB++) {
      // read one entry from decompressed stream.
      int res=in.read(buf,flen);
      if (res<flen) {
        throw UsrError("Premature end of xref stream");
      }

      xre_t xr;
      if (flens[0]>0) {
        unsigned int type=readBinary(buf,flens[0]);
        if (type>2) {
          xr.type=xre_t::XREF_UNKNOWN;
        } else {
          xr.type=(xre_t::Type)type;
        }
      } else {
        xr.type=xre_t::XREF_USED;
      }
  
      if (flens[1]>0) {
        xr.off=readBinary(buf+flens[0],flens[1]);
      } else {
        throw UsrError("No default value known");
      }

      if (flens[2]>0) {
        xr.gen=readBinary(buf+flens[0]+flens[1],flens[2]);
      } else if (xr.type==xre_t::XREF_USED) {
        xr.gen=0;
      } else {
        throw UsrError("No default value known");
      }

      if (offset+iB>=(int)to.size()) {
        to.resize(offset+iB+1,xre_t());
      }
      if (to[offset+iB].gen<xr.gen) {
        to[offset+iB]=xr;
      }
    }
  }
print(stdfo,false,false);

// TODO:  pdf.trdict._move_from(dict); ... parse_trailer_dict() ...

  return false;
}
// }} }

struct XRef::offset_sort { // {{{ [id] -> sort by [xref[id].off]
  offset_sort(const XRefVec &xref,const std::vector<int> &xrefpos)
             : xref(xref),xrefpos(xrefpos) {}
  bool operator()(int a, int b) const {
    if ( (a<0)&&(b<0) ) {
      return (xrefpos[~a]<xrefpos[~b]);
    } else if (a<0) {
      return (xrefpos[~a]<xref[b].off);
    } else if (b<0) {
      return (xref[a].off<xrefpos[~b]);
    }
    return (xref[a].off<xref[b].off);
  }
  const XRefVec &xref;
  const std::vector<int> &xrefpos;
};
// }}}

void XRef::generate_ends() // {{{
{
  std::vector<int> offset_keyed(xref.size()+xrefpos.size());

  // TRICK: all the xref-sections (usually only one), are handled with id <0  and taken care of at compare time
  //        they are needed to terminate the last object(s)  [pdf with appended update]
  for (int iA=0,val=-(int)xrefpos.size();iA<(int)offset_keyed.size();iA++,val++) {
    offset_keyed[iA]=val;
  }
  std::sort(offset_keyed.begin(),offset_keyed.end(),offset_sort(xref,xrefpos));

  // iterate the xref in desceding offset order
  int lastpos=-1;
  for (int iA=(int)offset_keyed.size()-1;iA>=0;iA--) {
    if (offset_keyed[iA]<0) {
      lastpos=xrefpos[~offset_keyed[iA]];
    } else if (xref[offset_keyed[iA]].type==xre_t::XREF_USED) {
      xref[offset_keyed[iA]].end=lastpos;
      lastpos=xref[offset_keyed[iA]].off;
    }
  }
}
// }}}

// zero padded!
long XRef::readUIntOnly(const char *buf,int len) // {{{
{
  long ret=0;

  for (int iA=0;iA<len;iA++) {
    if (!isdigit(buf[iA])) {
      return -1;
    }
    ret*=10;
    ret+=buf[iA]-'0';
  }
  return ret;
}
// }}}

long XRef::readBinary(const char *buf,int len) // {{{
{
  long ret=0;

  for (int iA=0;iA<len;iA++) {
    ret<<=8;
    ret|=(unsigned char)buf[iA];
  }
  return ret;
}
// }}}

// FIXME for stream.
void XRef::print(Output &out,bool as_stream,bool master) // {{{
{
  assert(!as_stream); // TODO
  static const char type_to_char[]={'f','n'};  // i.e. no compressed, not UNKNOWN
//  static const char type_to_char[]={'f','n','c','x'};

  const int xrsize=xref.size();
  out.puts("xref\n");
 
  int pos=0,end=xrsize;

  while (pos<xrsize) {
    if (!master) {
      for (;pos<xrsize;pos++) {
        if (xref[pos].gen!=-1) {
          break;
        }
      }
      for (end=pos;end<xrsize;end++) {
        if (xref[end].gen==-1) {
          break;
        }
      }
    }
    out.printf("%d %d\n",pos,end-pos);
    for (;pos<end;pos++) {
      assert(xref[pos].gen!=-1);
      assert(xref[pos].type<(int)sizeof(type_to_char));
      if ( (xref[pos].type==xre_t::XREF_USED)&&(xref[pos].off==-1) ) {
        printf("WARNING: preliminary ref %d not yet finished\n",pos);
      }
      out.printf("%010u %05d %c \n",xref[pos].off,xref[pos].gen,type_to_char[xref[pos].type]); 
    }
  }
}
// }}}

size_t XRef::size() const // {{{
{
  // ... update...
  return xref.size();
}
// }}}

long XRef::getStart(const Ref &ref) const // {{{
{
  if ( (ref.ref<0)||(ref.ref>=(int)xref.size())||(ref.gen<0) ) {
    return -1; // not found
  }
  if ( (xref[ref.ref].type==xre_t::XREF_FREE)||(xref[ref.ref].gen!=ref.gen) ) {
    return -1; // not there/ wrong generation(TODO?)
  }
  return xref[ref.ref].off;
}
// }}}

long XRef::getEnd(const Ref &ref) const // {{{
{
  if ( (ref.ref<0)||(ref.ref>=(int)xref.size())||(ref.gen<0) ) {
    return -1; // not found -> unknown
  }
  if ( (xref[ref.ref].type==xre_t::XREF_FREE)||(xref[ref.ref].gen!=ref.gen) ) {
    return -1; // not there/ wrong generation -> unknown
  }
  return xref[ref.ref].end;
}
// }}}

} // namespace PDFTools
