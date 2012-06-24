#include "rect.h"
#include <memory>
#include "exception.h"
#include "../objs/num.h"
#include "../objs/array.h"

#include "../io/base.h"

// #include "...." pdf 
#include "../objs/ptr.h"  // FIXME "contained" within PDF(?)

namespace PDFTools {

Rect::Rect()  // {{{
  : x1(0),y1(0),
    x2(0),y2(0)
{
}
// }}}

Rect::Rect(float x1,float y1,float x2,float y2) // {{{
  : x1(x1),y1(y1),
    x2(x2),y2(y2)
{
}
// }}}

#if 0
Rect::Rect(const Point &p1,const Point &p2) : x1(p1.x),y1(p1.y),x2(p2.x),y2(p2.y)
{
}

Rect::Rect(const Point &p) : x1(0),y1(0),x2(p.x),y2(p.y)
{
}
#endif

Rect::Rect(PDF &pdf,const Array &ary) // {{{
{
  if (ary.size()!=4) {
    throw UsrError("Array is not a Rectangle (wrong size)");
  }

  for (int iA=0;iA<4;iA++) {
    ObjectPtr ptr(ary.get(pdf,iA));
    const NumFloat *fval=dynamic_cast<const NumFloat *>(ptr.get());
    const NumInteger *ival=dynamic_cast<const NumInteger *>(ptr.get());
    if (fval) {
      operator[](iA)=fval->value();
    } else if (ival) {
      operator[](iA)=(float)ival->value();
    } else {
      throw UsrError("Array is not a Rectangle (wrong types)");
    }
  }
}
// }}}

float Rect::operator[](int pos) const // {{{
{
  if (pos==0) {
    return x1;
  } else if (pos==1) {
    return y1;
  } else if (pos==2) {
    return x2;
  } else if (pos==3) {
    return y2;
  } else {
    throw UsrError("Bad index for rect: %d",pos);
  }
}
// }}}

float &Rect::operator[](int pos) // {{{
{
  if (pos==0) {
    return x1;
  } else if (pos==1) {
    return y1;
  } else if (pos==2) {
    return x2;
  } else if (pos==3) {
    return y2;
  } else {
    throw UsrError("Bad index for rect: %d",pos);
  }
}
// }}}

Array *Rect::toArray() const // {{{
{
  std::auto_ptr<Array> ret(new Array);

  for (int iA=0;iA<4;iA++) {
    ret->add(new NumFloat((*this)[iA]),true);
  }
  return ret.release();
}
// }}}

void Rect::print(Output &out) const // {{{
{
  out.puts("[");
  fminout(out,x1);
  out.puts(" ");
  fminout(out,y1);
  out.puts(" ");
  fminout(out,x2);
  out.puts(" ");
  fminout(out,y2);
  out.puts("]");
//  out.printf("[%f %f %f %f]",x1,y1,x2,y2);
}
// }}}

} // namespace PDFTools
