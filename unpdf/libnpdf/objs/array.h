#ifndef UNPDF_OBJS_ARRAY_H
#define UNPDF_OBJS_ARRAY_H

#include "base.h"
#include "ptr.h"
#include <vector>

namespace PDFTools {

  class PDF;
  class Array : public Object {
  public:
    Array();
    ~Array();

    void add(const Object *obj,bool take=false);
    void print(Output &out) const;

    // clones recursively!
    Array *clone() const;
    const Object *operator[](int pos) const;
    size_t size() const;

    // advanced interface
    Object *get(int pos); // ensures ownership, allow modifications
    void set(int pos,const Object *obj,bool take); // replace(only!) [or change ownership]

    // convenience
    static Array *from(const std::vector<float> &nums);
    // these do not preserve Input position: !!!
    ObjectPtr get(PDF &pdf,int pos) const;
    ObjectPtr getTake(PDF &pdf,int pos); // transfer ownership
    std::string getString(PDF &pdf,int pos) const;
    std::vector<float> getNums(PDF &pdf,int num) const;

    DictPtr getDict(PDF &pdf,int pos,bool required=true) const; // not Dict: throws;  (!required && notfound): TPtr.empty()
    ArrayPtr getArray(PDF &pdf,int pos,bool required=true) const;

    int getUInt_D(int pos) const;

    void _move_from(Array *from);
    void _copy_from(const Array &from); // "deep copy"
  private:
    struct ArrayType {
      ArrayType(const Object *obj,bool ours);

      const Object *obj;
      bool ours;
    };
    typedef std::vector<ArrayType> ArrayVec;
    ArrayVec arvec;

    Array(const Array &);
    const Array &operator=(const Array &);
  };

} // namespace PDFTools

#endif
