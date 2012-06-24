#ifndef UNPDF_UTIL_UTIL_H
#define UNPDF_UTIL_UTIL_H

namespace PDFTools {

  class ParsingInput;
  void preprint(ParsingInput &in);

  class Object;
  void dump(const Object *obj);

  class Input;
  class Output;
  // copy >len bytes from >in to >out, len==-1: until EOF; DOES NOT flush
  int copy(Output &out,Input &in,int len=-1);

} // namespace PDFTools

#endif
