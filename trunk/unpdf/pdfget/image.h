#ifndef _IMAGE_H
#define _IMAGE_H

#include "libnpdf/io/base.h"
#include <vector>

namespace PDFTools {

  bool validBPC(int bpc);

  namespace DecodeFilter {
    class FInput : public Input {
    public:
      FInput(Input &read_from,int color,int bpc,const std::vector<float> *decode=NULL,bool invert=false);

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    protected:
      void reset();
    private:
      Input &read_from;
      int color,bpc;
      const std::vector<float> *decode;
      bool invert; // only 1color 1bit (TODO?)

      std::vector<char> block;
      int inpos;
    };
  } // namespace DecodeFilter

} // namespace PDFTools

#endif
