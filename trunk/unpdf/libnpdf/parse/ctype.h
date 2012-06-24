#ifndef UNPDF_PARSE_CTYPE_H
#define UNPDF_PARSE_CTYPE_H

namespace PDFTools {
namespace Parser {

  extern const char charType[256];
  inline bool is_delim(unsigned char c) { // space or special
    return (charType[c]&3);
  }
  inline bool is_space(unsigned char c) {
    return (charType[c]&1);
  }

} // namespace Parser
} // namespace PDFTools

#endif
