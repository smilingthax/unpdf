#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <stdexcept>

class UsrError : public std::exception {
public:
  UsrError(const char *fmt,...) throw();
  ~UsrError() throw();
  const char *what() const throw();
private:
  char *err;
};

#endif
