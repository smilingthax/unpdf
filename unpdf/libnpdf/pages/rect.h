#ifndef UNPDF_PAGES_RECT_H
#define UNPDF_PAGES_RECT_H

#include "../objs/base.h"

namespace PDFTools {

  class PDF;
  class Array;
  class Rect : public Object { // copyable, "an Array"
  public:
    Rect();
    Rect(float x1,float x2,float y1,float y2);
//    Rect(const Point &p1,const Point &p2);
//    Rect(const Point &p); // (0,0)-p
    Rect(PDF &pdf,const Array &ary);

    float operator[](int pos) const;
    Array *toArray() const;

    void print(Output &out) const;
  protected:
    float &operator[](int pos);
  private:
    float x1,y1,x2,y2;
  };

} // namespace PDFTools

#endif
