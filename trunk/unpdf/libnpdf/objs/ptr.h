#ifndef UNPDF_OBJS_PTR_H
#define UNPDF_OBJS_PTR_H

#include "base.h"

namespace PDFTools {

  struct ObjectPtr_ref { // helper,inline
    ObjectPtr_ref(Object *obj,bool take) : ptr(obj),ours(take) {}

    Object *ptr;
    bool ours;
  };

  class ObjectPtr { // an auto_ptr<Object>, that allows non-owned data, inline
  public:
    explicit ObjectPtr(Object *obj=0,bool take=true) : ptr(obj),ours(take) {}
    ~ObjectPtr() {
      if (ours) {
        delete ptr;
      }
    }
    ObjectPtr(ObjectPtr_ref ref) : ptr(ref.ptr),ours(ref.ours) {}

    operator ObjectPtr_ref() { return ObjectPtr_ref(release(),ours); }

    bool empty() const { return (ptr==NULL); }
    bool owns() const { return ours; }
    Object &operator*() const { return *ptr; }
    Object *operator->() const { return ptr; }
    Object *get() const { return ptr; }
    void reset(Object *obj=0,bool take=true);
    void reset(ObjectPtr_ref ref) { reset(ref.ptr,ref.ours); }
    Object *release() { // now your resposibility! check owns()
      Object *ret=ptr;
      ptr=NULL; // TODO: code depends on >ours not being resetted
      return ret;
    }

  private:
    Object *ptr;
    bool ours;

    ObjectPtr(ObjectPtr &optr);
    ObjectPtr &operator=(ObjectPtr &optr);
    ObjectPtr &operator=(ObjectPtr_ref ref);
  };

} // namespace PDFTools

#endif
