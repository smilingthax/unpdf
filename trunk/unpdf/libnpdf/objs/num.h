#ifndef UNPDF_OBJS_NUM_H
#define UNPDF_OBJS_NUM_H

#include "base.h"

namespace PDFTools {

  // output only the required number of digits of >val to >out
  void fminout(Output &out,float val);

  class Boolean : public Object {
  public:
    Boolean(bool value);
    void print(Output &out) const;
    Boolean *clone() const { return new Boolean(val); }

    bool value() const { return val; }
  private:
    bool val;
  };

  class NumInteger : public Object {
  public:
    NumInteger(int value=0);
    void print(Output &out) const;
    NumInteger *clone() const { return new NumInteger(val); }

    int value() const { return val; }
  private:
    int val;
  };

  class NumFloat : public Object {
  public:
    NumFloat(float value=0);
    void print(Output &out) const;
    NumFloat *clone() const { return new NumFloat(val); }

    float value() const { return val; }
  private:
    float val;
  };

} // namespace PDFTools

#endif
