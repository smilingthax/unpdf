#ifndef UNPDF_OBJS_PTR_H
#define UNPDF_OBJS_PTR_H

#include "base.h"

namespace PDFTools {

  template <typename T>
  struct TPtr_ref { // helper,inline
    TPtr_ref(T *ptr,bool take) : ptr(ptr),ours(take) {}

    T *ptr;
    bool ours;
  };

  class TPtr_base {
  public:
    static void assert_same(bool oldTake,bool newTake);
  };

  template <typename T>
  class TPtr : private TPtr_base { // an auto_ptr<Object>, that allows non-owned data, inline
  public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef value_type& reference;

    explicit TPtr(pointer ptr=0,bool take=true) : ptr(ptr),ours(take) {}
    ~TPtr() {
      if (ours) {
        delete ptr;
      }
    }
    TPtr(TPtr_ref<T> ref) : ptr(ref.ptr),ours(ref.ours) {}

    operator TPtr_ref<T>() { return TPtr_ref<T>(release(),ours); }

    bool empty() const { return (ptr==NULL); }
    bool owns() const { return ours; }

    reference operator*() const { return *ptr; }
    pointer operator->() const { return ptr; }
    pointer get() const { return ptr; }

    void reset(pointer nptr=0,bool take=true) {
      if (ptr!=nptr) {
        if (ours) {
          delete ptr;
        }
        ours=take;
        ptr=nptr;
      } else {
        assert_same(ours,take);
      }
    }
    void reset(TPtr_ref<T> ref) {
      reset(ref.ptr,ref.ours);
    }
    pointer release() { // now your resposibility! check owns()
      pointer ret=ptr;
      ptr=NULL; // TODO: code depends on >ours not being resetted
      return ret;
    }

  private:
    pointer ptr;
    bool ours;

    TPtr(TPtr &optr);
    TPtr &operator=(TPtr &optr);
    TPtr &operator=(TPtr_ref<T> ref);
  };

  class Array;
  class Dict;
  typedef TPtr<Object> ObjectPtr;
  typedef TPtr<Array> ArrayPtr;
  typedef TPtr<Dict> DictPtr;

} // namespace PDFTools

#endif
