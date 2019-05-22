#ifndef SHARED_ANYPTR_H_
#define SHARED_ANYPTR_H_

#include <memory>

// shared void *
class shared_anyptr {
public:
  shared_anyptr() = default;

  template <typename Access>
  shared_anyptr(const Access &access, typename Access::pointer p)
    : access(&access),
      ptr(std::shared_ptr<typename Access::value_type>(p))
  { }

  template <typename Access, typename Deleter>
  shared_anyptr(const Access &access, typename Access::pointer p, Deleter del)
    : access(&access),
      ptr(std::shared_ptr<typename Access::value_type>(p, std::move(del)))
  { }

  template <typename Access>
  shared_anyptr(const Access &access, const std::shared_ptr<typename Access::value_type> &rhs)
    : access(&access), ptr(rhs.ptr)
  { }

  shared_anyptr(const shared_anyptr &rhs)
    : access(rhs.access), ptr(rhs.ptr)
  { }

  shared_anyptr(shared_anyptr &&rhs) {
    swap(rhs);
  }

  shared_anyptr &operator=(const shared_anyptr &rhs) {
    access = rhs.access;
    ptr = rhs.ptr;
    return *this;
  }

  shared_anyptr &operator=(shared_anyptr &&rhs) {
    swap(rhs);
    return *this;
  }

  void swap(shared_anyptr &rhs) {
    std::swap(access, rhs.access);
    std::swap(ptr, rhs.ptr);
  }

  template <typename T>
  class Accessor;

  template <typename Access>
  typename Access::pointer get(const Access &_access) const {
    if (access != &_access) {
//      throw std::bad_cast();
      return nullptr;
    }
    return static_cast<typename Access::pointer>(ptr.get());
  }

  template <typename Access>
  void set(const Access &_access, const std::shared_ptr<typename Access::value_type> &_ptr) {
    access = &_access;
    ptr = _ptr;
  }

  template <typename Access>
  void set(const Access &_access, typename Access::pointer _ptr) {
    if (access == &_access && ptr.get() == _ptr) {
      return; // unchanged
    }
    access = &_access;
    ptr.reset(_ptr);
  }

  template <typename Access, typename Deleter>
  void set(const Access &_access, typename Access::pointer _ptr, Deleter del) {
    if (access == &_access && ptr.get() == _ptr) {
      // assert(std::get_deleter<void(*)(typename Access::pointer)>(ptr)==del...?);
      return; // unchanged
    }
    access = &_access;
    ptr.reset(_ptr, std::move(del));
  }

  template <typename Access>
  bool is(const Access &_access) const {
    return (access == &_access);
  }

  void reset() {
    access = nullptr;
    ptr.reset();
  }

/*
  template <typename T, typename Access, typename... Args>
  static shared_anyptr make(const Access &access, Args&&... args) {
    return shared_anyptr(access, std::make_shared<T>((Args&&)args...));
  }
*/

private:
  const void *access = nullptr;
  std::shared_ptr<void> ptr;
};

template <typename T>
class shared_anyptr::Accessor {
public:
  using value_type = T;
  using pointer = T*;

  pointer operator()(shared_anyptr &ptr) const {
    pointer ret = ptr.get(*this);
    if (!ret) {
      throw std::bad_cast();
    }
    return ret;
  }
};

#endif
