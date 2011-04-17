#ifndef _DIR_ITER_H
#define _DIR_ITER_H

#include <string>

class dir_iterator {
public:
  dir_iterator(const std::string &path);
  ~dir_iterator();

  const dir_iterator &operator++();
  const std::string &operator*() const;
  const std::string *operator->() const;
  bool end() const;
  std::string fullpath() const;
private:
  class dir_iterator_impl;
  dir_iterator_impl *impl;

  dir_iterator(const dir_iterator &);
  const dir_iterator &operator=(const dir_iterator &);
};

#endif
