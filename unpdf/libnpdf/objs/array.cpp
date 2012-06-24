#include "array.h"
#include <assert.h>
#include <memory>
#include "exception.h"
#include "../io/base.h"

#include "all.h" // as long as ptr will not help us

#include "../pdf/pdf.h"  // FIXME

namespace PDFTools {

Array::Array()
{
}

Array::~Array() // {{{
{
  for (ArrayVec::const_iterator it=arvec.begin();it!=arvec.end();++it) {
    if (it->ours) {
      delete it->obj;
    }
  }
}
// }}}

Array::ArrayType::ArrayType(const Object *obj,bool ours)
  : obj(obj),
    ours(ours)
{
}

void Array::add(const Object *obj,bool take) // {{{
{
  if (!obj) {
    throw std::invalid_argument("NULL pointer");
  }
  try {
    arvec.push_back(ArrayType(obj,take));
  } catch (...) {
    if (take) {
      delete obj;
    }
    throw;
  }
}
// }}}

void Array::print(Output &out) const // {{{
{
  out.puts("[");
  const int len=arvec.size();
  for (int iA=0;iA<len;iA++) {
    // even put one there for iA==0, as there's a special case where acro actually requires this
    out.puts(" ");
    arvec[iA].obj->print(out);
  }
  out.puts("]");
}
// }}}

Array *Array::clone() const // {{{
{
  std::auto_ptr<Array> ret(new Array);

  const int len=arvec.size();
  for (int iA=0;iA<len;iA++) {
    ret->add(arvec[iA].obj->clone(),true);
  }
  return ret.release();
}
// }}}

const Object *Array::operator[](int pos) const // {{{
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    assert(0);
    return NULL;
  }
  return arvec[pos].obj;
}
// }}}

size_t Array::size() const // {{{
{
  return arvec.size();
}
// }}}

Object *Array::get(int pos) // {{{
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    throw UsrError("Bad index for array: %d",pos);
  }
  if (!arvec[pos].ours) {
    throw UsrError("Modification of const Array entry attemped");
  }
  return const_cast<Object *>(arvec[pos].obj);
}
// }}}

void Array::set(int pos,const Object *obj,bool take) // {{{
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
// }}}

ObjectPtr Array::get(PDF &pdf,int pos) const // {{{
{
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    throw UsrError("Bad index for array: %d",pos);
  }
  return pdf.fetch(arvec[pos].obj);
}
// }}}

ObjectPtr Array::getTake(PDF &pdf,int pos) // {{{
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
// }}}

std::string Array::getString(PDF &pdf,int pos) const // {{{
{
  ObjectPtr fobj=get(pdf,pos);
  const String *sval=dynamic_cast<const String *>(fobj.get());
  if (!sval) {
    throw UsrError("Required string index %d not found",pos);
  }
  return std::string(sval->value()); // we must! copy
}
// }}}

Array *Array::getNums(const std::vector<float> &nums) // {{{
{
  std::auto_ptr<Array> ret(new Array);
  for (int iA=0;iA<(int)nums.size();iA++) {
    ret->add(new NumFloat(nums[iA]),true);
  }
  return ret.release();
}
// }}}

int Array::getUInt_D(int pos) const // {{{
{ 
  if ( (pos<0)||(pos>=(int)arvec.size()) ) {
    throw UsrError("Bad index for array: %d",pos);
  }
  const NumInteger *ival=dynamic_cast<const NumInteger *>(arvec[pos].obj);
  if ( (!ival)||(ival->value()<0) ) {
    throw UsrError("array pos %d is not an unsigned integer",pos);
  }
  return ival->value();
}
// }}}

std::vector<float> Array::getNums(PDF &pdf,int num) const // {{{
{
  if ((int)arvec.size()!=num) {
    throw UsrError("Wrong size for numeric array: %d (expected: %d)",arvec.size(),num);
  }
  std::vector<float> ret;
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
// }}}

void Array::_move_from(Array *from) // {{{
{
  assert(from);
  for (ArrayVec::iterator it=from->arvec.begin();it!=from->arvec.end();++it) {
    arvec.push_back(*it);
    it->ours=false;
  }
  from->arvec.clear();
}
// }}}

void Array::_copy_from(const Array &from) // {{{
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

} // namespace PDFTools
