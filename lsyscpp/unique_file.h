#pragma once

#include <stdio.h>
#include <memory>

namespace detail {
struct fclose_deleter {
  void operator()(FILE *ptr) const {
    fclose(ptr);
  }
};
} // namespace detail

struct unique_FILE : std::unique_ptr<FILE, detail::fclose_deleter> {
  unique_FILE(FILE *f = nullptr)
    : unique_ptr(f)
  { }

  unique_FILE(const char *name, const char *mode)
    : unique_FILE(fopen(name, mode))
  { }

  operator FILE *() const {
    return get();
  }

  using unique_ptr::reset;
  void reset(const char *name, const char *mode) {
    reset(fopen(name, mode));
  }
};

