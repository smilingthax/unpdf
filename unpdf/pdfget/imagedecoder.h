#ifndef _IMAGEDECODER_H
#define _IMAGEDECODER_H

struct ImageDecoder {
  ImageDecoder(int color,const float *decode=0)
    : color2(color*2),decode(decode),col(0)
  {}

  void setColorPos(int cpos);
  int getColorPos() const { return col; }

  void decode_f(int bpc,float *dst,const unsigned char *&src,int len) {
    decode_(bpc,dst,src,len);
  }
  void decode_u8(int bpc,unsigned char *dst,const unsigned char *&src,int len) {
    decode_(bpc,dst,src,len);
  }
  void encode_f(int bpc,unsigned char *&dst,const float *src,int len) {
    encode_(bpc,dst,src,len);
  }
  void encode_u8(int bpc,unsigned char *&dst,const unsigned char *src,int len) {
    encode_(bpc,dst,src,len);
  }

  // R = float  or unsigned char  or unsigned short
  // e.g. u8: this also 'encodes' by stretching to full u8 range
  template <typename R>
  void decode__1(R *dst,const unsigned char *&src,int len); // len in dsts, should be full input byte
  template <typename R>
  void decode__2(R *dst,const unsigned char *&src,int len);
  template <typename R>
  void decode__4(R *dst,const unsigned char *&src,int len);
  template <typename R>
  void decode__8(R *dst,const unsigned char *&src,int len);
  template <typename R>
  void decode__16(R *dst,const unsigned char *&src,int len);
  template <typename R>
  void decode_(int bpc,R *dst,const unsigned char *&src,int len);

  static void invert_1(unsigned char *dst,const unsigned char *&src,int len); // len in bits
  static unsigned char *invert_1_inplace(unsigned char *buf,int len);

  // NOTE: for /Indexed: don't convert to float at all... (but still apply decode!)
  static void get_u8_1(unsigned char *dst,const unsigned char *&src,int len);
  static void get_u8_2(unsigned char *dst,const unsigned char *&src,int len);
  static void get_u8_4(unsigned char *dst,const unsigned char *&src,int len);

  template <typename R>
  void encode__1(unsigned char *&dst,const R *src,int len);
  template <typename R>
  void encode__2(unsigned char *&dst,const R *src,int len);
  template <typename R>
  void encode__4(unsigned char *&dst,const R *src,int len);
  template <typename R>
  void encode__8(unsigned char *&dst,const R *src,int len);
  template <typename R>
  void encode__16(unsigned char *&dst,const R *src,int len);
  template <typename R>
  void encode_(int bpc,unsigned char *&dst,const R *src,int len);

  // no decode, i.e.
  static void put_u8_1(unsigned char *&dst,const unsigned char *src,int len);
  static void put_u8_2(unsigned char *&dst,const unsigned char *src,int len);
  static void put_u8_4(unsigned char *&dst,const unsigned char *src,int len);
  // 8: no-op
  // 16: not possible

/*
_cmyk_to_rgb
_gray_to_rgb
*/

  static float interp(int val,int maxval,float dmin,float dmax);
  static int interp_int(int val,int maxval_in,int maxval_out,float dmin,float dmax);

  static int quantize(float val,int maxval,float dmin,float dmax);
  static int quantize_int(int val,int maxval_in,int maxval_out,float dmin,float dmax);

  int color2; // bpc;
  const float *decode;
  int col;
};

#endif
