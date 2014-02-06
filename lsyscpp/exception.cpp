#include "exception.h"
#include "tools.h"

using namespace std;

// {{{ UsrError
UsrError::UsrError(const char *_err,...) throw()
{
  va_list ap;

  va_start(ap,_err);
  try {
    err=a_vsprintf(_err,ap);
  } catch(...) {
    err=NULL;
  }
  va_end(ap);
}

UsrError::~UsrError() throw()
{
  free(err);
}

const char *UsrError::what() const throw()
{
  return err;
}
// }}}
