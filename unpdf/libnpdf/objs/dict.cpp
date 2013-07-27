#include "dict.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory>
#include "exception.h"
#include "../io/base.h"

#include "../pdf/pdf.h"  // FIXME
#include "../objs/string.h"
#include "../objs/array.h"

namespace PDFTools {

Dict::Dict()
{
}

Dict::~Dict()
{
  clear();
}

Dict::DictType::DictType(const char *_key,const Object *obj,bool ours) // {{{
  : obj(obj),
    ours(ours) 
{
  key=strdup(_key);
  if (!key) {
    if (ours) {
      delete obj;
    }
    throw std::bad_alloc();
  }
}
// }}}

void Dict::clear() // {{{
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
// }}}

bool Dict::empty() const // {{{
{
  return dict.empty();
}
// }}}

size_t Dict::size() const // {{{
{
  return dict.size();
}
// }}}

void Dict::print(Output &out) const // {{{
{
  out.puts("<<\n");
  for (DictMap::const_iterator it=dict.begin();it!=dict.end();++it) {
    const DictType &dt=it->second;
    out.printf("  /%s ",dt.key);
    dt.obj->print(out);
    out.puts("\n");
  }
  out.puts(">>");
}
// }}}

Dict *Dict::clone() const // {{{
{
  std::auto_ptr<Dict> ret(new Dict);

  ret->_copy_from(*this);
  return ret.release();
}
// }}}

Dict::const_iterator Dict::begin() const // {{{
{
  return const_iterator(dict.begin());
}
// }}}

Dict::const_iterator Dict::end() const // {{{
{
  return const_iterator(dict.end());
}
// }}}

// TODO FIXME: this will create duplicates
void Dict::add(const char *key,const Object *obj,bool take) // {{{
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
    dict.insert(std::make_pair(hash(key),DictType(key,obj,take)));
  } catch (...) {
    if (take) {
      delete obj;
    }
    throw;
  }
}
// }}}

const Object *Dict::find(const char *key) const // {{{
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
// }}}

Dict::DictMap::iterator Dict::fnd(const char *key) // {{{
{
  if (!key) {
    throw std::invalid_argument("NULL pointer");
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
// }}}

void Dict::erase(const char *key) // {{{
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
// }}}

unsigned int Dict::hash(const char *str) const // {{{
{
  unsigned int ret=0;
  for (;*str;++str) {
    ret=*str^((ret>>3)+(ret<<5));
  }
  return ret;
}
// }}}

Object *Dict::get(const char *key) // {{{
{
  DictMap::iterator it=fnd(key);
  if (it==dict.end()) {
    throw UsrError("Bad key /%s",key);
  }
  if (!it->second.ours) {
    throw UsrError("Modification of const Dict entry attempted");
  }
  return const_cast<Object *>(it->second.obj);
}
// }}}

void Dict::set(const char *key,const Object *obj,bool take) // {{{
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
// }}}

void Dict::_move_from(Dict *from) // {{{
{
  assert(from);
  for (DictMap::iterator it=from->dict.begin();it!=from->dict.end();) {
    dict.insert(*it);
    from->dict.erase(it++);
  }
}
// }}}

void Dict::_copy_from(const Dict &from) // {{{
{
  for (DictMap::const_iterator it=from.dict.begin();it!=from.dict.end();++it) {
    const Object *nobj=it->second.obj->clone();
    try {
      dict.insert(std::make_pair(it->first,DictType(it->second.key,nobj,true)));
    } catch (...) {
      delete nobj;
      throw;
    }
  }
}
// }}}

void Dict::ensure(PDF &pdf,const char *key,const char *name) const // {{{
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
// }}}

ObjectPtr Dict::get(PDF &pdf,const char *key) const // {{{
{
  const Object *obj=find(key);
  if (!obj) {
    return ObjectPtr();
  }
  return pdf.fetch(obj);
}
// }}}

int Dict::getInt(PDF &pdf,const char *key,bool *found) const // {{{
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
// }}}

int Dict::getInt(PDF &pdf,const char *key,int defval) const // {{{
{
  bool found;
  int ret=getInt(pdf,key,&found);
  if (!found) {
    return defval;
  }
  return ret;
}
// }}}

float Dict::getNum(PDF &pdf,const char *key,float defval) const // {{{
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
// }}}

bool Dict::getBool(PDF &pdf,const char *key,bool defval) const // {{{
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
// }}}

int Dict::getNames(PDF &pdf,const char *key,const char *name0,const char *name1,.../*name1,...,NULL*/) const // {{{
{
  const Name &nval=getName(pdf,key,(name0==NULL));
  int ret=0;

  if (nval.empty()) {
    return ret;
  } else if ( (name0)&&(strcmp(nval.value(),name0)==0) ) {
    return ret;
  }
  ret++;

  va_list ap;
  const char *tmp=name1;
  va_start(ap,name1);
  while (tmp) {
    if (strcmp(nval.value(),tmp)==0) {
      va_end(ap);
      return ret;
    }
    tmp=va_arg(ap,const char *);
    ret++;
  }
  va_end(ap);

  throw UsrError("Name /%s for key /%s is not allowed",nval.value(),key);
}
// }}}

Name_ref Dict::getName(PDF &pdf,const char *key,bool required) const // {{{
{
  ObjectPtr fobj=get(pdf,key);
  if (fobj.empty()) {
    if (required) {
      throw UsrError("Required name key /%s not found",key);
    }
    return Name_ref();
  }
  Name *nval=dynamic_cast<Name *>(fobj.get());
  if (!nval) {
    throw UsrError("/%s is not a Name",key);
  }
  if (!fobj.owns()) {
    return Name(nval->value(),Name::STATIC);
  }
  if (const char *val=nval->release()) {
    return Name(val,Name::TAKE);
  }
  return Name(nval->value(),Name::DUP);
}
// }}}

std::string Dict::getString(PDF &pdf,const char *key) const // {{{
{
  ObjectPtr fobj=get(pdf,key);
  const String *sval=dynamic_cast<const String *>(fobj.get());
  if (!sval) {
    throw UsrError("Required string key /%s not found",key);
  }
  return std::string(sval->value()); // we must! copy
}
// }}}

DictPtr Dict::getDict(PDF &pdf,const char *key,bool required) const // {{{
{
  ObjectPtr fobj=get(pdf,key);
  if (fobj.empty()) {
    if (required) {
      throw UsrError("Required dict key /%s not found",key);
    }
    return DictPtr();
  }
  Dict *dval=dynamic_cast<Dict *>(fobj.get());
  if (!dval) {
    throw UsrError("/%s is not a Dict",key);
  }
  fobj.release();
  return DictPtr(dval,fobj.owns());
}
// }}}

ArrayPtr Dict::getArray(PDF &pdf,const char *key,bool required) const // {{{
{
  ObjectPtr fobj=get(pdf,key);
  if (fobj.empty()) {
    if (required) {
      throw UsrError("Required array key /%s not found",key);
    }
    return ArrayPtr();
  }
  Array *aval=dynamic_cast<Array *>(fobj.get());
  if (!aval) {
    throw UsrError("/%s is not an Array",key);
  }
  fobj.release();
  return ArrayPtr(aval,fobj.owns());
}
// }}}

bool Dict::getBool_D(const char *key,bool defval) const // {{{
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
// }}}

int Dict::getInt_D(const char *key,int defval) const // {{{
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
// }}}

const char *Dict::getName_D(const char *key) const // {{{
{
  const Object *obj=find(key);
  if (!obj) {
    return NULL;
  }
  const Name *nval=dynamic_cast<const Name *>(obj);
  if (!nval) {
    throw UsrError("/%s is not a Name",key);
  }
  return nval->value();
}
// }}}

// {{{ Dict::const_iterator
ObjectPtr Dict::const_iterator::get(PDF &pdf) const
{
  return pdf.fetch(it->second.obj);
}
// }}}

} // namespace PDFTools
