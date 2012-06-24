#include "rand.h"
#include <stdio.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include "exception.h"

namespace PDFTools {

std::string RAND::get(int len) // {{{
{
  std::string ret;
  ret.resize(len);

  if (RAND_bytes((unsigned char *)&ret[0],len)==0) {
    if (RAND_pseudo_bytes((unsigned char *)&ret[0],len)==0) {
      throw UsrError("PRNG failed: %ld",ERR_get_error());
    }
    fprintf(stderr,"Warning: non-secure PRNG used\n");
  }

  return ret;
}
// }}}

} // namespace PDFTools
