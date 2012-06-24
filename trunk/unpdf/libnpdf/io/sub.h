#ifndef UNPDF_IO_SUB_H
#define UNPDF_IO_SUB_H

#include "base.h"

namespace PDFTools {

  class SubInput : public Input {
  public:
    // endpos can be -1
    SubInput(Input &read_from,long startpos,long endpos);

    bool empty() const;
    long basepos() const;

    int read(char *buf,int len);
    long pos() const;
    void pos(long pos);
  private:
    SubInput *parent; // when nested
    Input &read_from;
    long startpos,endpos,cpos;
  };

} // namespace PDFTools

#endif
