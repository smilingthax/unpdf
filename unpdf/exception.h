#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <stdexcept>
#include <string>

class UsrError : public std::exception {
public:
  UsrError(const char *fmt,...) throw();
  ~UsrError() throw();
  const char *what() const throw();
private:
  char *err;
};

class FS_except : public std::exception {
public:
  FS_except(int errnum) throw();
  ~FS_except() throw() {}
  const char *what() const throw() { return errtext.c_str(); }
  int err_no() const throw() { return errnum; }
private:
  std::string errtext;
  int errnum;
};

#endif
