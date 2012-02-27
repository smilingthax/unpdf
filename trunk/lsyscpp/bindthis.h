#ifndef BIND_THIS_H_
#define BIND_THIS_H_

// use as:
//   YourClass instance;
//   auto tmi=bind_this(&YourClass::TheMethod,&instance);
//   ...tmi(...);

template <typename T,typename R,typename... Args>
struct BoundThis {
  BoundThis(R (T::*pmf)(Args...),T *This) : This(This),pmf(pmf) {}

  R operator()(Args&&... args) const {
    return (This->*pmf)(std::forward<Args>(args)...);
  }

private:
  T *This;
  R (T::*pmf)(Args...);
};

template<class T,class R,typename... Args>
BoundThis<T,R,Args...> bind_this(R (T::*pmf)(Args...),T *This)
{
  return BoundThis<T,R,Args...>(pmf,This);
}

template<class T,class R,typename... Args>
BoundThis<const T,R,Args...> bind_this(R (T::*pmf)(Args...)const,const T *This)
{
  return BoundThis<const T,R,Args...>(pmf,This);
}

#endif
