#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <typeinfo>
#include "pdfio.h"
#include "pdfbase.h"
#include "pdfparse.h"
#include "pdfcomp.h"
#include "pdfsec.h"
#include "exception.h"

using namespace PDFTools;
using namespace std;

#define EPS 1e-8
FILEOutput stdfo(stdout);

// {{{ PDFTools::fminout(Output &out,float val)  - output only the required number of digits of >val to >out
void PDFTools::fminout(Output &out,float val)
{
  // output only required number of digits
  float ival=truncf(val),fval=fabsf(val-ival);
  out.printf("%.0f",ival);
  if (fval>EPS) {
    out.puts(".");
    float tmp=fval;
    while (tmp>EPS) {
      fval*=10;
      out.printf("%d",(int)fmodf(fval,10));
      tmp/=10;
    }
  }
}
// }}}

/* TODO : FILEInput
// {{{ copy(Output &out,FILE *f,len)  - copy >len bytes from >f to >out
void copy(Output &out,FILE *f,int len)
{
  assert( (f)&&(len>=0) );
  char buf[4096];
  int iA;
 
  while (len>0) {
    if (len>4096) {
      iA=fread(buf,1,4096,f);
    } else {
      iA=fread(buf,1,len,f);
    }
    if (iA<=0) {
      assert(0);
      break;
    }
    out.write(buf,iA);
    len-=iA;
  }
}
// }}}
*/

// {{{ PDFTools::copy(Output &out,Input &in,len)  - copy >len bytes from >in to >out, len==-1: until EOF; DOES NOT flush
int PDFTools::copy(Output &out,Input &in,int len)
{
  char buf[4096];
  int iA;

  if (len<0) {
    len=0;
    do {
      iA=in.read(buf,4096);
      out.write(buf,iA);
      len+=iA;
    } while (iA>0);
    return len;
  }
  int ret=0;
  while (len>0) {
    if (len>=4096) {
      iA=in.read(buf,4096);
    } else {
      iA=in.read(buf,len);
    }
    if (iA<=0) {
      break;
    }
    out.write(buf,iA);
    len-=iA;
    ret+=iA;
  }
  return ret;
}
// }}}


// {{{ PDFTools::Object
void PDFTools::Object::print(Output &out) const
{
  out.puts("null");
}

Object *PDFTools::Object::clone() const
{
  if (!isnull(this)) {
    throw UsrError("clone() not implemented for %s",typeid(*this).name());
  }
  return new Object();
}
// }}}

// {{{ PDFTools::ObjectPtr
PDFTools::ObjectPtr::ObjectPtr(ObjectPtr &optr) : ptr(optr.release()),ours(optr.ours)
{
}

ObjectPtr &PDFTools::ObjectPtr::operator=(ObjectPtr &optr)
{
  reset(optr.release(),optr.ours);
  return *this;
}

PDFTools::ObjectPtr::ObjectPtr &PDFTools::ObjectPtr::operator=(ObjectPtr_ref ref)
{
  reset(ref.ptr,ref.ours);
  return *this;
}

void PDFTools::ObjectPtr::reset(Object *obj,bool take)
{
  if (obj!=ptr) {
    if (ours) {
      delete ptr;
    }
    ours=take;
    ptr=obj;
  } else {
    assert(ours==take);
  }
}
// }}}

// {{{ PDFTools::Boolean
PDFTools::Boolean::Boolean(bool value) : val(value)
{
}

void PDFTools::Boolean::print(Output &out) const
{
  if (val) {
    out.puts("true");
  } else {
    out.puts("false");
  }
}
// }}}

// {{{ PDFTools::NumInteger
PDFTools::NumInteger::NumInteger(int value) : val(value)
{
}

void PDFTools::NumInteger::print(Output &out) const
{
  out.printf("%d",val);
}
// }}}

// {{{ PDFTools::NumFloat
PDFTools::NumFloat::NumFloat(float value) : val(value)
{
}

void PDFTools::NumFloat::print(Output &out) const
{
  fminout(out,val);
//  out.printf("%f",val);
}
// }}}

// {{{ PDFTools::String
PDFTools::String::String() : val(),as_hex(false)
{
}

PDFTools::String::String(const char *str) : val(str),as_hex(false)
{
}

PDFTools::String::String(const char *str,int len,bool as_hex) : val(str,len),as_hex(as_hex)
{
}

PDFTools::String::String(const std::string &str,const Decrypt *decrypt,bool _as_hex)
{
  if (decrypt) {
    (*decrypt)(val,str);
  } else {
    val.assign(str);
  }
  as_hex=_as_hex;
}

void PDFTools::String::print(Output &out) const
{
// TODO: if (output==Encrypt_Output) ...
  if (as_hex) {
    out.put('<');
    for (int iA=0;iA<(int)val.size();iA++) {
      if ( (iA)&&(iA%40==0) ) {
        out.put('\n');
      }
      out.printf("%02x",(unsigned char)val[iA]);
    }
    out.put('>');
  } else {
    out.put('(');
    // escape special chars: \0 \\ \)
    const char *buf=val.data();
    int iA=0;
    for (int iB=0;iB<(int)val.size();iA++,iB++) {
      if ( (buf[iA]==0)||(buf[iA]==')')||(buf[iA]=='\\') ) {
        out.write(buf,iA);
        if (buf[iA]==0) {
          out.write("\\000",4);
        } else {
          out.put('\\');
          out.put(buf[iA]);
        }
        buf+=iA+1;
        iA=-1;
      }
    }
    out.write(buf,iA);
//    out.write(val.data(),val.size()); // TODO? maybe wrap line after...
    out.put(')');
  }
}
// }}}

// {{{ PDFTools::Name
PDFTools::Name::Name(const char *name, FreeT ft)
{
  if (ft!=DUP) {
    assert(name);
    val=name;
    ours=(ft==TAKE);
    return;
  }
  val=strdup(name);
  ours=true;
  if (!val) {
    throw bad_alloc();
  }
}

PDFTools::Name::~Name()
{
  if (ours) {
    free((void *)val);
  }
}

void PDFTools::Name::print(Output &out) const
{
  out.put('/');
  for (int iA=0;val[iA];iA++) {
    if ( (iA=='#')||(Parser::is_delim(val[iA])) ) {
      out.printf("#%02x",val[iA]);
      // TODO:  Test it!
      // const char hex[]="0123456789abcdef";
      // maybe: out.put('#'); 
      // out.put(hex[(val[iA]>>4)&0xf]);
      // out.put(hex[val[iA]&0xf]);
    } else {
      out.put(val[iA]);
    }  
  }
//  out.printf("/%s",val);
}
// }}}

// {{{ PDFTools::Array
PDFTools::Array::Array()
{
}

PDFTools::Array::~Array()
{
  for (ArrayVec::const_iterator it=arvec.begin();it!=arvec.end();++it) {
    if (it->ours) {
      delete it->obj;
    }
  }
}

PDFTools::Array::ArrayType::ArrayType(const Object *obj,bool ours) : obj(obj), ours(ours)
{
}

void PDFTools::Array::add(const Object *obj,bool take)
{
  assert(obj);
  try {
    arvec.push_back(ArrayType(obj,take));
  } catch (...) {
    if (take) {
      delete obj;
    }
    throw;
  }
}

void PDFTools::Array::print(Output &out) const
{
  out.puts("[");
  const int no=arvec.size();
  for (int iA=0;iA<no;iA++) {
    if (iA) {
      out.puts(" ");
    }
    arvec[iA].obj->print(out);
  }
  out.puts("]");
}

Array *PDFTools::Array::clone() const
{
  auto_ptr<Array> ret(new Array);

  const int no=arvec.size();
  for (int iA=0;iA<no;iA++) {
    ret->add(arvec[iA].obj->clone(),true);
  }
  return ret.release();
}

const Object *PDFTools::Array::operator[](int pos) const
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    assert(0);
    return NULL;
  }
  return arvec[pos].obj;
}

unsigned int PDFTools::Array::size() const
{
  return arvec.size();
}

Object *PDFTools::Array::get(int pos)
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    throw UsrError("Bad index for array: %d",pos);
  }
  if (!arvec[pos].ours) {
    throw UsrError("Modification of const Array entry attemped");
  }
  return const_cast<Object *>(arvec[pos].obj);
}

void PDFTools::Array::set(int pos,const Object *obj,bool take)
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    if (take) {
      delete obj;
    }
    throw UsrError("Bad index for array: %d",pos);
  }
  if (arvec[pos].ours) {
    delete arvec[pos].obj;
  }
  arvec[pos].obj=obj;
  arvec[pos].ours=take;
}

ObjectPtr PDFTools::Array::get(PDF &pdf,int pos) const
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    throw UsrError("Bad index for array: %d",pos);
  }
  return pdf.fetch(arvec[pos].obj);
}

ObjectPtr PDFTools::Array::getTake(PDF &pdf,int pos)
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    throw UsrError("Bad index for array: %d",pos);
  }
  ObjectPtr ret=pdf.fetch(arvec[pos].obj);
  if ( (!ret.owns())&&(arvec[pos].ours) ) {
    arvec[pos].ours=false;
    return ObjectPtr(ret.release(),true);
  }
  return ObjectPtr(ret.release(),ret.owns());
}

string PDFTools::Array::getString(PDF &pdf,int pos) const
{
  ObjectPtr fobj=get(pdf,pos);
  const String *sval=dynamic_cast<const String *>(fobj.get());
  if (!sval) {
    throw UsrError("Required string index %d not found",pos);
  }
  return string(sval->value()); // we must! copy
}

Array *PDFTools::Array::getNums(const vector<float> &nums)
{
  auto_ptr<Array> ret(new Array);
  for (int iA=0;iA<(int)nums.size();iA++) {
    ret->add(new NumFloat(nums[iA]),true);
  }
  return ret.release();
}

vector<float> PDFTools::Array::getNums(PDF &pdf,int num) const
{
  if ((int)arvec.size()!=num) {
    throw UsrError("Wrong size for numeric array: %d (expected: %d)",arvec.size(),num);
  }
  vector<float> ret;
  ret.resize(arvec.size(),0);
  for (int iA=0;iA<(int)arvec.size();iA++) {
    ObjectPtr aobj=pdf.fetch(arvec[iA].obj);
    if (const NumInteger *ni=dynamic_cast<const NumInteger *>(aobj.get())) {
      ret[iA]=ni->value();
    } else if (const NumFloat *nf=dynamic_cast<const NumFloat *>(aobj.get())) {
      ret[iA]=nf->value();
    } else {
      throw UsrError("Not a number in numeric array");
    }
  }
  return ret;
}

void PDFTools::Array::_move_from(Array *from)
{
  assert(from);
  for (ArrayVec::iterator it=from->arvec.begin();it!=from->arvec.end();++it) {
    arvec.push_back(*it);
    it->ours=false;
  }
  from->arvec.clear();
}

void PDFTools::Array::_copy_from(const Array &from)
{
  for (ArrayVec::const_iterator it=from.arvec.begin();it!=from.arvec.end();++it) {
    const Object *nobj=it->obj->clone();
    try {
      arvec.push_back(ArrayType(nobj,true));
    } catch (...) {
      delete nobj;
      throw;
    }
  }
}
// }}}

// {{{ PDFTools::Ref
void PDFTools::Ref::print(Output &out) const
{
  out.printf("%d %d R",ref,gen);
}
// }}}

// {{{ PDFTools::Dict
PDFTools::Dict::Dict()
{
}

PDFTools::Dict::~Dict()
{
  clear();
}

PDFTools::Dict::DictType::DictType(const char *_key,const Object *obj,bool ours) : obj(obj), ours(ours) 
{
  key=strdup(_key);
  if (!key) {
    if (ours) {
      delete obj;
    }
    throw bad_alloc();
  }
}

void PDFTools::Dict::clear()
{
  for (DictMap::const_iterator it=dict.begin();it!=dict.end();++it) {
    const DictType &dt=it->second;
    free(dt.key);
    if (dt.ours) {
      delete dt.obj;
    }
  }
  dict.clear();
}

bool PDFTools::Dict::empty() const
{
  return dict.empty();
}

unsigned int PDFTools::Dict::size() const
{
  return dict.size();
}

void PDFTools::Dict::print(Output &out) const
{
  out.puts("<<\n");
  for (DictMap::const_iterator it=dict.begin();it!=dict.end();++it) {
    const DictType &dt=it->second;
    out.printf("/%s ",dt.key);
    dt.obj->print(out);
    out.puts("\n");
  }
  out.puts(">>");
}

Dict *PDFTools::Dict::clone() const
{
  auto_ptr<Dict> ret(new Dict);

  ret->_copy_from(*this);
  return ret.release();
}

Dict::const_iterator PDFTools::Dict::begin() const
{
  return const_iterator(dict.begin());
}

Dict::const_iterator PDFTools::Dict::end() const
{
  return const_iterator(dict.end());
}

void PDFTools::Dict::add(const char *key,const Object *obj,bool take)
{
  if ( (!key)||(!obj) ) {
    if (take) {
      delete obj;
    }
    assert(false);
    return;
  }
  if (isnull(obj)) { // null-object means: no entry
    return;
  }

  try {
    dict.insert(make_pair(hash(key),DictType(key,obj,take)));
  } catch (...) {
    if (take) {
      delete obj;
    }
    throw;
  }
}

const Object *PDFTools::Dict::find(const char *key) const
{
  if (!key) {
    return NULL;
  }
  const unsigned int hs=hash(key);
  DictMap::const_iterator it=dict.find(hs);
  if (it==dict.end()) {
    return NULL;
  }
  while (it->first==hs) {
    const DictType &dt=it->second;
    if (strcmp(dt.key,key)==0) {
      return dt.obj;
    }
    ++it;
  }
  return NULL;
}

Dict::DictMap::iterator PDFTools::Dict::fnd(const char *key)
{
  if (!key) {
    throw invalid_argument("NULL pointer");
  }
  const unsigned int hs=hash(key);
  DictMap::iterator it=dict.find(hs);
  if (it!=dict.end()) {
    while (it->first==hs) {
      if (strcmp(it->second.key,key)==0) {
        return it;
      }
      ++it;
    }
  }
  return dict.end();
}

void PDFTools::Dict::erase(const char *key)
{
  DictMap::iterator it=fnd(key);
  if (it==dict.end()) {
    return;
  }
  if (it->second.ours) {
    delete it->second.obj;
  }
  dict.erase(it);
}

unsigned int PDFTools::Dict::hash(const char *str) const
{
  unsigned int ret=0;
  for (;*str;++str) {
    ret=*str^((ret>>3)+(ret<<5));
  }
  return ret;
}

Object *PDFTools::Dict::get(const char *key)
{
  DictMap::iterator it=fnd(key);
  if (it==dict.end()) {
    throw UsrError("Bad key /%s",key);
  }
  if (!it->second.ours) {
    throw UsrError("Modification of const Dict entry attemped");
  }
  return const_cast<Object *>(it->second.obj);
}

void PDFTools::Dict::set(const char *key,const Object *obj,bool take)
{
  DictMap::iterator it=fnd(key);
  if (it==dict.end()) {
    if (take) {
      delete obj;
    }
    throw UsrError("Bad key /%s",key);
  }
  if ( (it->second.ours)&&(it->second.obj!=obj) ) {
    delete it->second.obj;
  }
  it->second.obj=obj;
  it->second.ours=take;
}

void PDFTools::Dict::_move_from(Dict *from)
{
  assert(from);
  for (DictMap::iterator it=from->dict.begin();it!=from->dict.end();) {
    dict.insert(*it);
    from->dict.erase(it++);
  }
}

void PDFTools::Dict::_copy_from(const Dict &from)
{
  for (DictMap::const_iterator it=from.dict.begin();it!=from.dict.end();++it) {
    const Object *nobj=it->second.obj->clone();
    try {
      dict.insert(make_pair(it->first,DictType(it->second.key,nobj,true)));
    } catch (...) {
      delete nobj;
      throw;
    }
  }
}

void PDFTools::Dict::ensure(PDF &pdf,const char *key,const char *name)
{
  const Object *obj=find(key);
  if (!obj) {
    throw UsrError("Required key /%s not found",key);
  }

  ObjectPtr fobj=pdf.fetch(obj);
  const Name *nval=dynamic_cast<const Name *>(fobj.get());
  if (!nval) {
    throw UsrError("/%s is not a Name",key);
  }
  if (strcmp(nval->value(),name)!=0) {
    throw UsrError("Expected /%s to be %s, but %s found",key,name,nval->value()); 
  }
}

ObjectPtr PDFTools::Dict::get(PDF &pdf,const char *key) const
{
  const Object *obj=find(key);
  if (!obj) {
    return ObjectPtr();
  }
  return pdf.fetch(obj);
}

int PDFTools::Dict::getInt(PDF &pdf,const char *key,bool *found) const 
{ 
  const Object *obj=find(key);
  if (!obj) {
    if (!found) {
      throw UsrError("Required integer key /%s not found",key);
    } 
    *found=false;
    return 0;
  } 
  ObjectPtr fobj=pdf.fetch(obj);
  const NumInteger *nival=dynamic_cast<const NumInteger *>(fobj.get());
  if (!nival) {
    if (!found) {
      throw UsrError("Key /%s is not integer",key);
    }
    *found=false;
    return 0;
  } else if (found) {
    *found=true;
  }
  return nival->value();
}

int PDFTools::Dict::getInt(PDF &pdf,const char *key,int defval) const
{
  bool found;
  int ret=getInt(pdf,key,&found);
  if (!found) {
    return defval;
  }
  return ret;
}

float PDFTools::Dict::getNum(PDF &pdf,const char *key,float defval) const
{
  ObjectPtr aobj=get(pdf,key);
  if (aobj.empty()) {
    return defval;
  }
  if (const NumInteger *ni=dynamic_cast<const NumInteger *>(aobj.get())) {
    return ni->value();
  } else if (const NumFloat *nf=dynamic_cast<const NumFloat *>(aobj.get())) {
    return nf->value();
  } else {
    throw UsrError("/%s is not a Number",key);
  }
}

bool PDFTools::Dict::getBool(PDF &pdf,const char *key,bool defval) const
{
  ObjectPtr fobj=get(pdf,key);
  if (fobj.empty()) {
    return defval;
  }
  const Boolean *bval=dynamic_cast<const Boolean *>(fobj.get());
  if (!bval) {
    throw UsrError("/%s is not a Boolean",key);
  }
  return bval->value();
}

int PDFTools::Dict::getNames(PDF &pdf,const char *key,const char *name0,const char *name1,.../*name1,...,NULL*/) const
{
  ObjectPtr fobj=get(pdf,key);
  if (fobj.empty()) {
    if (!name0) {
      throw UsrError("Required name key /%s not found",key);
    }
    return 0;
  }
  const Name *nval=dynamic_cast<const Name *>(fobj.get());
  if (!nval) {
    throw UsrError("/%s is not a Name",key);
  }
  int ret=0;

  if ( (name0)&&(strcmp(nval->value(),name0)==0) ) {
    return ret;
  }
  ret++;

  va_list ap;
  const char *tmp=name1;
  va_start(ap,name1);
  while (tmp) {
    if (strcmp(nval->value(),tmp)==0) {
      va_end(ap);
      return ret;
    }
    tmp=va_arg(ap,const char *);
    ret++;
  }
  va_end(ap);

  throw UsrError("Name /%s for key /%s is not allowed",nval->value(),key);
}

string PDFTools::Dict::getString(PDF &pdf,const char *key) const
{
  ObjectPtr fobj=get(pdf,key);
  const String *sval=dynamic_cast<const String *>(fobj.get());
  if (!sval) {
    throw UsrError("Required string key /%s not found",key);
  }
  return string(sval->value()); // we must! copy
}

bool PDFTools::Dict::getBool_D(const char *key,bool defval) const
{
  const Object *obj=find(key);
  if (!obj) {
    return defval;
  }
  const Boolean *bval=dynamic_cast<const Boolean *>(obj);
  if (!bval) {
    throw UsrError("/%s is not a Boolean",key);
  }
  return bval->value();
}

int PDFTools::Dict::getInt_D(const char *key,int defval) const
{
  const Object *obj=find(key);
  if (!obj) {
    return defval;
  }
  const NumInteger *ival=dynamic_cast<const NumInteger *>(obj);
  if (!ival) {
    throw UsrError("/%s is not an Integer",key);
  }
  return ival->value();
}

// {{{ Dict::const_iterator
ObjectPtr PDFTools::Dict::const_iterator::get(PDF &pdf) const
{
  return pdf.fetch(it->second.obj);
}
// }}}
// }}}
 
// {{{ PDFTools::XRef
PDFTools::XRef::XRef(bool writeable)
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

Ref PDFTools::XRef::newRef(long pos)
{
  assert(xrefpos.empty()); // create-mode
  Ref ret(xref.size());
  xref.push_back(xre_t(pos));
  return ret;
}

void PDFTools::XRef::setRef(const Ref &ref,long pos)
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

void PDFTools::XRef::clear() 
{
  assert(xrefpos.empty()); // create-mode
  xref.clear();
}

bool PDFTools::XRef::parse(ParsingInput &pi)
{
  assert(!xrefpos.empty()); // non-create-mode
  // .. if a as_stream - TODO
  long xrp=pi.pos();
  bool ret=read_xref(pi,xref);
  if (ret) {
    if (xrefpos[0]==-1) {
      xrefpos[0]=xrp;
    } else {
      xrefpos.push_back(xrp);
    }
  }
  generate_ends(); // TODO? optimize/ only once
  return ret;
}

bool PDFTools::XRef::read_xref(ParsingInput &fi,XRefVec &to) // {{{
{
  if (!fi.next("xref")) {
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

struct PDFTools::XRef::offset_sort { // {{{ [id] -> sort by [xref[id].off]
  offset_sort(const XRefVec &xref,const vector<int> &xrefpos)
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
  const vector<int> &xrefpos;
};
// }}}

void PDFTools::XRef::generate_ends()
{
  vector<int> offset_keyed(xref.size()+xrefpos.size());

  // TRICK: all the xref-sections (usually only one), are handled with id <0  and taken care of at compare time
  //        they are needed to terminate the last object(s)  [pdf with appended update]
  for (int iA=0,val=-(int)xrefpos.size();iA<(int)offset_keyed.size();iA++,val++) {
    offset_keyed[iA]=val;
  }
  sort(offset_keyed.begin(),offset_keyed.end(),offset_sort(xref,xrefpos));

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

long PDFTools::XRef::readUIntOnly(const char *buf,int len)
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

void PDFTools::XRef::print(Output &out,bool as_stream)
{
  assert(!as_stream); // TODO
  const char type_to_char[]={'f','n'}; 

  const int xrsize=xref.size();
  out.printf("xref\n"
             "0 %d\n",xrsize);
  for (int iA=0;iA<xrsize;iA++) {
    assert(xref[iA].gen!=-1);
    assert(xref[iA].type<(int)sizeof(type_to_char));
    if ( (xref[iA].type==xre_t::XREF_USED)&&(xref[iA].off==-1) ) {
      printf("WARNING: preliminary ref %d not yet finished\n",iA);
    }
    out.printf("%010u %05d %c \n",xref[iA].off,xref[iA].gen,type_to_char[xref[iA].type]); 
  }
}

size_t PDFTools::XRef::size() const 
{
  // ... update...
  return xref.size();
}

long PDFTools::XRef::getStart(const Ref &ref) const
{
  if ( (ref.ref<0)||(ref.ref>=(int)xref.size())||(ref.gen<0) ) {
    return -1; // not found
  }
  if ( (xref[ref.ref].type==xre_t::XREF_FREE)||(xref[ref.ref].gen!=ref.gen) ) {
    return -1; // not there/ wrong generation(TODO?)
  }
  return xref[ref.ref].off;
}

long PDFTools::XRef::getEnd(const Ref &ref) const
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

// {{{ PDFTools::PDF
PDFTools::PDF::PDF(Input &read_base,int version,int xrefpos) : read_base(read_base),version(version),xref(false),security(NULL)
{
  // read xref and trailer
  read_base.pos(xrefpos);

  ParsingInput pi(read_base);
  read_xref_trailer(pi);

  // read rootdict
  const Object *pobj;
  if ((pobj=trdict.find("Root"))==NULL) {
    throw UsrError("No Root entry in trailer");
  }
  auto_ptr<Object> robj(fetch(dynamic_cast<const Ref &>(*pobj))); // throws if not a Ref (must be indirect)

  Dict *rdict=dynamic_cast<Dict *>(robj.get());
  if (!rdict) {
    throw UsrError("Root catalog is not a dictionary");
  }
  rootdict._move_from(rdict);
  rootdict.ensure(*this,"Type","Catalog");

  // version update?
  pobj=rootdict.find("Version");
  if (pobj) { // version update
    // TODO : may be indirect
    const Name *nval=dynamic_cast<const Name *>(pobj);
    if (!nval) {
      throw UsrError("/Version value is not a Name");
    }
    const char *val=nval->value();
    if ( (!isdigit(val[0]))||(val[1]!='.')||(!isdigit(val[2]))||(val[3]) ) {
      throw UsrError("/Version is not a pdf version");
    }
    int newver=(val[0]-'0')*10+(val[2]-'0');
    if (newver<version) {
      throw UsrError("New /Version %d is older than current version %d",newver,version);
    }
    version=newver;
  }

  // get Document ID
  ObjectPtr iobj=trdict.get(*this,"ID");
  if (!iobj.empty()) {
    const Array *aval=dynamic_cast<const Array *>(iobj.get());
    if ( (!aval)||(aval->size()!=2) ) {
      throw UsrError("/ID is not an Array of length 2");
    }
    fileid.first=aval->getString(*this,0);
    fileid.second=aval->getString(*this,1);
  }

  // Encryption
  pobj=trdict.find("Encrypt");
  if (pobj) {
    robj.reset(fetch(dynamic_cast<const Ref &>(*pobj))); // throws if not a Ref

    rdict=dynamic_cast<Dict *>(robj.get());
    if (!rdict) {
      throw UsrError("/Encrypt is not a dictionary");
    }

    int filtertype=rdict->getNames(*this,"Filter",NULL,"Standard",NULL);
    assert(filtertype==1);
    //  throw UsrError("Unsupported Security Handler /%s",nval->value());
    security=new StandardSecurityHandler(*this,fileid.first,rdict);
  }

  try {
    // pages tree
    const Ref *pgref=dynamic_cast<const Ref *>(rootdict.find("Pages"));
    if (!pgref) {
      throw UsrError("No /Pages reference found in root catalog");
    }
    pages.parse(*this,*pgref);
//    dump(pages[0].Content);

  } catch (...) {
    delete security;
    throw;
  }
}

PDFTools::PDF::~PDF()
{
  delete security;
}

Decrypt *PDFTools::PDF::getStmDecrypt(const Ref &ref)
{
  if (!security) {
    return NULL;
  }
  return security->getDecrypt(ref,StandardSecurityHandler::StmF);
}

Decrypt *PDFTools::PDF::getStrDecrypt(const Ref &ref)
{
  if (!security) {
    return NULL;
  }
  return security->getDecrypt(ref,StandardSecurityHandler::StrF);
}

Decrypt *PDFTools::PDF::getEffDecrypt(const Ref &ref)
{
  if (!security) {
    return NULL;
  }
  return security->getDecrypt(ref,StandardSecurityHandler::Eff);
}

Object *PDFTools::PDF::getObject(const Ref &ref)
{
  int start=xref.getStart(ref);
  if (start==-1)  { // not found: Null-object
    return new Object();
  }
  SubInput si(read_base,start,xref.getEnd(ref));
 
  return Parser::parseObj(*this,si,&ref);
}

Object *PDFTools::PDF::fetch(const Ref &ref)
{
  auto_ptr<Object> obj(getObject(ref));

  Ref *refval;
  while ((refval=dynamic_cast<Ref *>(obj.get()))!=NULL) {
    obj.reset(getObject(*refval));
  }
  return obj.release();
}

Object *PDFTools::PDF::fetchP(const Ref &ref) const
{
  Input &rb=const_cast<Input &>(read_base);
  long spos=rb.pos();
  Object *ret=const_cast<PDF &>(*this).fetch(ref);
  rb.pos(spos);

  return ret;
}

ObjectPtr PDFTools::PDF::fetch(const Object *obj)
{
  if (!obj) {
    throw invalid_argument("NULL pointer");
  }
  const Ref *refval=dynamic_cast<const Ref *>(obj);
  if (refval) {
    return ObjectPtr(fetch(*refval),true);
  }
  return ObjectPtr(const_cast<Object *>(obj),false);
}

void PDFTools::PDF::read_xref_trailer(ParsingInput &pi) // {{{
{
  if (!xref.parse(pi)) {
    throw UsrError("Could not read xref");
  }

  // reference says: "trailer precedes startxref" and not "trailer follows xref"; but even acrobat seems to do this one instead
  pi.skip(false); // some pdfs require this

  if (!pi.next("trailer")) {
    throw UsrError("Could not read trailer");
  }
  pi.skip();
  auto_ptr<Dict> tr_dict(Parser::parseDict(pi));
  trdict._move_from(tr_dict.get());
  
  const Object *pobj=trdict.find("Prev");
  while (pobj) {
    const NumInteger *ival=dynamic_cast<const NumInteger*>(pobj);
    if (!ival) {
      throw UsrError("/Prev value is not Integer");
    }
    pi.pos(ival->value());
    if (!xref.parse(pi)) {
      throw UsrError("Could not read previous xref");
    }
    if (!pi.next("trailer")) {
      throw UsrError("Could not read previous trailer");
    }
    pi.skip();
    tr_dict.reset(Parser::parseDict(pi));
    pobj=tr_dict->find("Prev");
  }

  // check size
  pobj=trdict.find("Size");
  if (!pobj) {
    throw UsrError("No /Size in trailer");
  }
  const NumInteger *ival=dynamic_cast<const NumInteger*>(pobj);
  if (!ival) {
    throw UsrError("/Size in trailer is not an Integer");
  }
  if (ival->value()!=(int)xref.size()) {
    throw UsrError("Damaged trailer or xref (size does not match)");
  }
}
// }}}

// }}}

bool PDFTools::isnull(const Object *obj) // {{{
{
  if (obj) {
    return (typeid(*obj)==typeid(const Object));
  }
  printf("WARNING: isnull(NULL)\n");
  return false;
}
// }}}
