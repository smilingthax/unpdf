#ifndef UNPDF_CRYPT_RAND_H
#define UNPDF_CRYPT_RAND_H

#include <string>

namespace PDFTools {

  class RAND {
  public:
    static std::string get(int len);
  };

} // namespace PDFTools

#endif
