#ifndef _IMAGE_H
#define _IMAGE_H

#include "libnpdf/io/base.h"
#include <vector>

  #include "imagedecoder.h"

namespace PDFTools {

  bool validBPC(int bpc);

  namespace DecodeFilter {
    class FInput : public Input {
    public:
      FInput(Input &read_from,int width,int color,int bpc,const std::vector<float> *decode,bool invert=false);

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    protected:
      void reset();
    private:
      Input &read_from;
      int color,bpc,widthp,wpos;
      bool invert; // only 1color 1bit (TODO?)
      ImageDecoder idec;

      std::vector<unsigned char> block;
      const unsigned char *inpos,*end;
      char o_buf[8],*o_pos,*o_end;
    };
  } // namespace DecodeFilter

  namespace InvertFilter { // only for 1bpp
    class FInput : public Input {
    public:
      FInput(Input &read_from,int width,bool invert=true);

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    private:
      Input &read_from;
      int widthp,wpos;
      bool invert;
    };
  } // namespace DecodeFilter

} // namespace PDFTools

#endif
