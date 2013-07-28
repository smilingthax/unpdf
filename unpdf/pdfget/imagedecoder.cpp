#include "imagedecoder.h"
#include <string.h>
#include <assert.h>

// explicit instantiation
template void ImageDecoder::decode_<float>(int,float *,const unsigned char *&,int);
template void ImageDecoder::decode_<unsigned char>(int,unsigned char *,const unsigned char *&,int);
template void ImageDecoder::decode_<unsigned short>(int,unsigned short *,const unsigned char *&,int);

template void ImageDecoder::encode_<float>(int,unsigned char *&,const float *,int);
template void ImageDecoder::encode_<unsigned char>(int,unsigned char *&,const unsigned char *,int);
template void ImageDecoder::encode_<unsigned short>(int,unsigned char *&,const unsigned short *,int);


// image_decoder_trait<T> {{{
template <typename T>
struct image_decoder_trait {};

template <>
struct image_decoder_trait<float> {
  static float interp1(bool val,float dmin,float dmax) {
    return (val) ? dmax : dmin;
  }
  static float interp(int val,int maxval,float dmin,float dmax) {
    return ImageDecoder::interp(val,maxval,dmin,dmax);
  }
  static int quantize(float val,int maxval,float dmin,float dmax) {
    return ImageDecoder::quantize(val,maxval,dmin,dmax);
  }
};

template <>
struct image_decoder_trait<unsigned char> {
  static const int scale = 0xff;
  static unsigned char interp1(bool val,float dmin,float dmax) {
    return ((val) ? dmax : dmin)*scale;
  }
  static unsigned char interp(int val,int maxval,float dmin,float dmax) {
    return ImageDecoder::interp_int(val,maxval,scale,dmin,dmax);
  }
  static int quantize(unsigned char val,int maxval,float dmin,float dmax) {
    return ImageDecoder::quantize_int(val,scale,maxval,dmin,dmax);
  }
};

template <>
struct image_decoder_trait<unsigned short> {
  static const int scale = 0xffff;
  static unsigned short interp1(bool val,float dmin,float dmax) {
    return ((val) ? dmax : dmin)*scale;
  }
  static unsigned short interp(int val,int maxval,float dmin,float dmax) {
    return ImageDecoder::interp_int(val,maxval,scale,dmin,dmax);
  }
  static int quantize(unsigned short val,int maxval,float dmin,float dmax) {
    return ImageDecoder::quantize_int(val,scale,maxval,dmin,dmax);
  }
};
// }}}


void ImageDecoder::setColorPos(int cpos) // {{{
{
  assert( (cpos>=0)&&(2*cpos<color2) );
  col=cpos;
}
// }}}

float ImageDecoder::interp(int val,int maxval,float dmin,float dmax) // {{{
{
  float ret=dmin+val*(dmax-dmin)/maxval;
  if (ret<dmin) {
    return dmin;
  } else if (ret>dmax) {
    return dmax;
  }
  return ret;
}
// }}}

int ImageDecoder::interp_int(int val,int maxval_in,int maxval_out,float dmin,float dmax) // {{{
{
  int ret=dmin+val*(dmax-dmin)*maxval_out/maxval_in;
  if (ret<0) {
    return 0;
  } else if (ret>maxval_out) {
    return maxval_out;
  }
  return ret;
}
// }}}

int ImageDecoder::quantize(float val,int maxval,float dmin,float dmax) // {{{
{
  // TODO ? lrintf()?
  int ret=(0.5f+(val-dmin)*maxval/(dmax-dmin));
  if (ret<0) {
    return 0;
  } else if (ret>maxval) {
    return maxval;
  }
  return ret;
}
// }}}

int ImageDecoder::quantize_int(int val,int maxval_in,int maxval_out,float dmin,float dmax) // {{{
{
  int ret=(0.5f+(val-dmin)*maxval_out/((dmax-dmin)*maxval_in));
  if (ret<0) {
    return 0;
  } else if (ret>maxval_out) {
    return maxval_out;
  }
  return ret;
}
// }}}


template <typename R>
void ImageDecoder::decode__1(R *dst,const unsigned char *&src,int len) // {{{
{
  assert(decode);

  int bit=7;
  for (;len>0;len--) {
    *dst=image_decoder_trait<R>::interp1((*src>>bit)&0x01,decode[col],decode[col+1]);
    dst++;
    if (!bit) {
      bit=7;
      src++;
    } else {
      bit--;
    }
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
}
// }}}

template <typename R>
void ImageDecoder::decode__2(R *dst,const unsigned char *&src,int len) // {{{
{
  assert(decode);

  int bit=6;
  for (;len>0;len--) {
    *dst=image_decoder_trait<R>::interp((*src>>bit)&0x03,0x03,decode[col],decode[col+1]);
    dst++;
    if (!bit) {
      bit=6;
      src++;
    } else {
      bit-=2;
    }
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
}
// }}}

template <typename R>
void ImageDecoder::decode__4(R *dst,const unsigned char *&src,int len) // {{{
{
  assert(decode);

  int bit=0;
  for (;len>0;len--) {
    bit^=1;
    if (bit) {
      *dst=image_decoder_trait<R>::interp((*src>>4)&0x0f,0x0f,decode[col],decode[col+1]);
    } else {
      *dst=image_decoder_trait<R>::interp((*src)&0x0f,0x0f,decode[col],decode[col+1]);
      src++;
    }
    dst++;
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
}
// }}}

template <typename R>
void ImageDecoder::decode__8(R *dst,const unsigned char *&src,int len) // {{{
{
  assert(decode);

  for (;len>0;len--) {
    *dst=image_decoder_trait<R>::interp(*src,0xff,decode[col],decode[col+1]);
    dst++;
    src++;
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
}
// }}}

template <typename R>
void ImageDecoder::decode__16(R *dst,const unsigned char *&src,int len) // {{{
{
  assert(decode);

  for (;len>0;len--) {
    *dst=image_decoder_trait<R>::interp((src[0]<<8)|src[1],0xffff,decode[col],decode[col+1]);
    dst++;
    src+=2;
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
}
// }}}

template <typename R>
void ImageDecoder::decode_(int bpc,R *dst,const unsigned char *&src,int len) // {{{
{
  switch (bpc) {
  case 1:
    decode__1(dst,src,len);
    break;
  case 2:
    decode__2(dst,src,len);
    break;
  case 4:
    decode__4(dst,src,len);
    break;
  case 8:
    decode__8(dst,src,len);
    break;
  case 16:
    decode__16(dst,src,len);
    break;
  default:
    assert(0);
    break;
  }
}
// }}}


void ImageDecoder::invert_1(unsigned char *dst,const unsigned char *&src,int len) // {{{
{
  for (;len>7;len-=8) {
    *dst=(*src)^0xff;
    dst++;
    src++;
  }
  if (len>0) {
    *dst=(*src)^(0xff<<(8-len));
    src++;
  }
}
// }}}

unsigned char *ImageDecoder::invert_1_inplace(unsigned char *buf,int len) // {{{
{
  for (;len>7;len-=8) {
    (*buf)^=0xff;
    buf++;
  }
  if (len>0) {
    (*buf)^=(0xff<<(8-len));
    buf++;
  }
  return buf;
}
// }}}


void ImageDecoder::get_u8_1(unsigned char *dst,const unsigned char *&src,int len) // {{{
{
  int bit=7;
  for (;len>0;len--) {
    if (*src&(1<<bit)) { // 1
      *dst=1;
    } else { // 0
      *dst=0;
    }
    dst++;
    if (!bit) {
      bit=7;
      src++;
    } else {
      bit--;
    }
  }
  if (bit) {
    src++;
  }
}
// }}}

void ImageDecoder::get_u8_2(unsigned char *dst,const unsigned char *&src,int len) // {{{
{
  for (;len>3;len-=4,src++) {
    *dst=(*src>>6)&0x03;
    dst++;
    *dst=(*src>>4)&0x03;
    dst++;
    *dst=(*src>>2)&0x03;
    dst++;
    *dst=(*src>>0)&0x03;
    dst++;
  }
  const unsigned char c=*src;
  src++;
  if (len<=0) return;
  *dst=(c>>6)&0x03;
  dst++;
  if (len==1) return;
  *dst=(c>>4)&0x03;
  dst++;
  if (len==2) return;
  *dst=(c>>2)&0x03;
  dst++;
}
// }}}

void ImageDecoder::get_u8_4(unsigned char *dst,const unsigned char *&src,int len) // {{{
{
  for (;len>1;len-=2) {
    *dst=(*src>>4)&0x0f;
    dst++;
    src++;
    *dst=(*src)&0x0f;
    dst++;
    src++;
  }
  if (len>0) {
    *dst=(*src>>4)&0x0f;
    src++;
  }
}
// }}}


template <typename R>
void ImageDecoder::encode__1(unsigned char *&dst,const R *src,int len) // {{{
{
  assert(decode);

  if (len==0) {
    return;
  }

  int bit=7;
  unsigned char c=0;
  for (;len>0;len--) {
    c|=image_decoder_trait<R>::quantize(*src,0x01,decode[col],decode[col+1])<<bit;
    src++;
    if (!bit) {
      bit=7;
      *dst=c;
      dst++;
      c=0;
    } else {
      bit--;
    }
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
  if (bit) {
    *dst=c;
    dst++;
  }
}
// }}}

template <typename R>
void ImageDecoder::encode__2(unsigned char *&dst,const R *src,int len) // {{{
{
  assert(decode);

  if (len==0) {
    return;
  }

  int bit=6;
  unsigned char c=0;
  for (;len>0;len--) {
    c|=image_decoder_trait<R>::quantize(*src,0x03,decode[col],decode[col+1])<<bit;
    src++;
    if (!bit) {
      bit=6;
      *dst=c;
      dst++;
      c=0;
    } else {
      bit-=2;
    }
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
  if (bit) {
    *dst=c;
    dst++;
  }
}
// }}}

template <typename R>
void ImageDecoder::encode__4(unsigned char *&dst,const R *src,int len) // {{{
{
  assert(decode);

  if (len==0) {
    return;
  }

  int bit=0;
  unsigned char c=0;
  for (;len>0;len--) {
    bit^=1;
    if (bit) {
      c=image_decoder_trait<R>::quantize(*src,0x0f,decode[col],decode[col+1])<<4;
    } else {
      *dst=c|image_decoder_trait<R>::quantize(*src,0x0f,decode[col],decode[col+1]);
      dst++;
    }
    src++;
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
  if (!bit) {
    *dst=c;
    dst++;
  }
}
// }}}

template <typename R>
void ImageDecoder::encode__8(unsigned char *&dst,const R *src,int len) // {{{
{
  assert(decode);

  for (;len>0;len--) {
    *dst=image_decoder_trait<R>::quantize(*src,0xff,decode[col],decode[col+1]);
    dst++;
    src++;
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
}
// }}}

template <typename R>
void ImageDecoder::encode__16(unsigned char *&dst,const R *src,int len) // {{{
{
  assert(decode);

  for (;len>0;len--) {
    int c=image_decoder_trait<R>::quantize(*src,0x0ffff,decode[col],decode[col+1]);
    src++;
    *dst=c>>8;
    dst++;
    *dst=c&0xff;
    dst++;
    col+=2;
    if (col>=color2) {
      col=0;
    }
  }
}
// }}}

template <typename R>
void ImageDecoder::encode_(int bpc,unsigned char *&dst,const R *src,int len) // {{{
{
  switch (bpc) {
  case 1:
    encode__1(dst,src,len);
    break;
  case 2:
    encode__2(dst,src,len);
    break;
  case 4:
    encode__4(dst,src,len);
    break;
  case 8:
    encode__8(dst,src,len);
    break;
  case 16:
    encode__16(dst,src,len);
    break;
  default:
    assert(0);
    break;
  }
}
// }}}


void ImageDecoder::put_u8_1(unsigned char *&dst,const unsigned char *src,int len) // {{{
{
  int bit=7;
  unsigned char c=0;
  for (;len>0;len--,src++) {
    c|=(*src&0x01)<<bit;
    if (!bit) {
      bit=7;
      *dst=c;
      dst++;
      c=0;
    } else {
      bit--;
    }
  }
  if (bit) {
    *dst=c;
    src++;
  }
}
// }}}

void ImageDecoder::put_u8_2(unsigned char *&dst,const unsigned char *src,int len) // {{{
{
  for (;len>3;len-=4,dst++) {
    unsigned char c;
    c=(*src&0x03)<<6;
    src++;
    c|=(*src&0x03)<<4;
    src++;
    c|=(*src&0x03)<<2;
    src++;
    c|=(*src&0x03)<<0;
    src++;
    *dst=c;
  }
  unsigned char c;
  switch (len) {
  case 3:
    c=(*src&0x03)<<6;
    src++;
    c|=(*src&0x03)<<4;
    src++;
    c|=(*src&0x03)<<2;
    break;
  case 2:
    c=(*src&0x03)<<6;
    src++;
    c|=(*src&0x03)<<4;
    break;
  case 1:
    c=(*src&0x03)<<6;
    break;
  case 0:
  default:
    return;
  }
  *dst=c;
  dst++;
}
// }}}

void ImageDecoder::put_u8_4(unsigned char *&dst,const unsigned char *src,int len) // {{{
{
  for (;len>1;len-=2,src+=2) {
    *dst=((src[0]&0x0f)<<4)|(src[1]&0x0f);
    dst++;
  }
  if (len) {
    *dst=(*src&0x0f)<<4;
    dst++;
  }
}
// }}}

