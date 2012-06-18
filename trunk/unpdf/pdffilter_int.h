#ifndef _PDFFILTER_INT_H
#define _PDFFILTER_INT_H

#include <vector>
#include "pdffilter.h"
#include <zlib.h>

typedef struct LZWSTATE LZWSTATE;
typedef struct G4STATE G4STATE;
typedef struct jpeg_compress_struct * j_compress_ptr;
typedef struct jpeg_decompress_struct * j_decompress_ptr;

namespace PDFTools {
  namespace AHexFilter {
    extern const char *name;

    class FInput : public Input {
    public:
      FInput(Input &read_from);

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    protected:
      void reset();
    private:
      Input &read_from;
      std::vector<char> block;
      int inpos;
    };
    class FOutput : public Output {
    public:
      FOutput(Output &write_to);

      void write(const char *buf,int len);
      void flush();
    private:
      Output &write_to;
      std::vector<char> block;
    };

    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter);
  };
  namespace A85Filter {
    extern const char *name;

    class FInput : public Input {
    public:
      FInput(Input &read_from);

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    protected:
      void reset();
    private:
      Input &read_from;
      std::vector<char> block;
      int inpos;
      unsigned int ubuf;
      int ulen;
    };
    class FOutput : public Output {
    public:
      FOutput(Output &write_to);
    
      void write(const char *buf,int len);
      void flush();
    private:
      Output &write_to;
      unsigned int ubuf;
      int ulen;
      int linelen;
    };

    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter);
  };
  class PredInput : public Input {
  public:
    PredInput(Input *read_from,int width,int color,int bpp,int predictor); // takes; >predictor!=1
    ~PredInput();

    int read(char *buf,int len);
    long pos() const;
    void pos(long pos); // pos(0): "reset"

  protected:
    void tiff_decode();
    void png_decode(int type);
  private:
    Input *read_from;
    std::vector<unsigned char> line[2];
    unsigned char *lastline,*thisline;
    int cpos;
    int width,color,bpp;
    int pbyte,pshift;
    bool ispng;
  };
  class PredOutput : public Output {
  public:
    PredOutput(Output *write_to,int width,int color,int bpp,int predictor); // takes, >predictor!=1
    ~PredOutput();

    void write(const char *buf,int len);
    void flush();

  protected:
    void tiff_encode();
    void png_encode(unsigned char *dest,int type);
    int png_encode(); // ... TODO: encode best

  private:
    Output *write_to;
    std::vector<unsigned char> line[2];
    std::vector<std::vector<unsigned char> > encd;
    unsigned char *lastline,*thisline;
    int cpos;
    int width,color,bpp;
    int pbyte,pshift;
    bool ispng;
  };
  namespace LZWFilter {
    extern const char *name;

    struct Params {
      Params();
      Params(const Dict &params);
      Dict *getDict() const;

      int predictor,color,bpc,width;
      int early; // only lzw
    };

    class FInput : public Input {
    public:
      FInput(Input &read_from,int early=1);
      ~FInput();

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    private:
      Input &read_from;
      LZWSTATE *state;
      bool eof;
    };
    class FOutput : public Output {
    public:
      FOutput(Output &write_to,int early=1);
      ~FOutput();
    
      void write(const char *buf,int len);
      void flush();
    private:
      Output &write_to;
      LZWSTATE *state;
    };

    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter,const Params &prm=Params());
  };
  namespace FlateFilter {
    extern const char *name;

    using LZWFilter::Params;

    class FInput : public Input {
    public: 
      FInput(Input &read_from);
      ~FInput();

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos); // not good
    private:
      Input &read_from;
      std::vector<char> buf;
      z_stream zstr;
    };
    class FOutput : public Output {
    public: 
      FOutput(Output &write_to);
      ~FOutput();

      void write(const char *buf,int len);
      void flush();
    private:
      Output &write_to;
      std::vector<char> buf;
      z_stream zstr;
    };

    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter,const Params &prm=Params());
  };
  namespace RLEFilter {
    extern const char *name;

    class FInput : public Input {
    public:
      FInput(Input &read_from);

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    protected:
      void reset();
    private:
      Input &read_from;
      int runlen; // <0: repeat(>lastchar) of length (-runlen), ==0: read next token, >0: verbatim of length (runlen), -129 -> EOD
      char lastchar;
    };
    class FOutput : public Output {
    public:
      FOutput(Output &write_to);
 
      void write(const char *buf,int len);
      void flush();
    private:
      Output &write_to;
      int runlen;
      char lastchar;
      std::vector<char> verbbuf;
    };

    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter);
  };
  namespace FaxFilter {
    extern const char *name;

    struct Params {
      Params();
      Params(const Dict &params);
      Dict *getDict() const;

      int width,kval;
      bool invert;

      bool eol,eba,eob;
      int maxdamage;
    };

    class FInput : public Input {
    public:
      FInput(Input &read_from,int kval,int width=1728,bool invert=false);
      ~FInput();

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);
    private:
      Input &read_from;
      G4STATE *state;
      std::vector<char> outbuf;
      int outpos;
      bool invert;
      bool eof;
    };
    class FOutput : public Output {
    public:
      FOutput(Output &write_to,int kval,int width=1728,bool invert=false);
      ~FOutput();

      void write(const char *buf,int len);
      void flush();
    private:
      Output &write_to;
      G4STATE *state;
      std::vector<char> inbuf;
      int inpos;
      bool invert;
    };

    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter,const Params &prm=Params());
  };
  namespace JBIG2Filter {
    extern const char *name;

    struct Params {};
    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter,const Params &prm=Params());
  };
  namespace JpegFilter {
    extern const char *name;

    struct Params {
      Params();
      Params(const Dict &params);
      Dict *getDict() const;

      // encode
      int quality; // 0..100
      int width,height,color; // bpc always 8
      // en-/decode
      int colortransform;
    };

    class FInput : public Input {
    public:
      FInput(Input &read_from,int colortransform=-1);
      ~FInput();

      int read(char *buf,int len);
      long pos() const;
      void pos(long pos);

      void get_params(int &width,int &height,int &color);
    private:
      Input &read_from;
      j_decompress_ptr cinfo;
      int colortransform;
      std::vector<char> outbuf;
      int outpos;
    };
    class FOutput : public Output {
    public:
      FOutput(Output &write_to,int quality,int width,int height,int color,int colortransform=-1);
      ~FOutput();

      void write(const char *buf,int len);
      void flush();
    private:
      Output &write_to;
      j_compress_ptr cinfo;
      int width,color;
      std::vector<char> inbuf;
      int inpos;
    };
    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter,const Params &prm=Params());
  };
  // PDF-1.5
  namespace JPXFilter {
    extern const char *name;
    Input *makeInput(const char *fname,Input &read_from,const Dict *params);
    void makeOutput(OFilter &filter);
  };
};

#endif
