#include "image.h"
 #include "exception.h"
 #include <assert.h>
 #include <string.h>

namespace PDFTools {

bool validBPC(int bpc) // {{{
{
  return (bpc==1)||(bpc==2)||(bpc=4)||(bpc==8)||
         (/*pdf16*/bpc==16);
}
// }}}


// TODO? ColorSpace::getDefaultDecode(std::vector<float> &ret)
/*
*/
// .. do not read   char*   but  float*  ?

#define BLOCK_SIZE 1024
DecodeFilter::FInput::FInput(Input &read_from,int width,int color,int bpc,const std::vector<float> *decode,bool invert) // {{{
  : read_from(read_from),
    color(color),bpc(bpc),
    widthp(width*color),wpos(widthp),
    invert(invert),
    idec(color,(decode)? &decode->front() : 0),
    o_pos(o_buf+sizeof(o_buf)/sizeof(*o_buf)),o_end(o_pos)
{
  assert(widthp>0);
  block.resize(BLOCK_SIZE);
  end=&block.back();
  inpos=end;
  assert( (!invert)||(color*bpc==1) );
}
// }}}

int DecodeFilter::FInput::read(char *buf,int len) // {{{
{
  int olen=0;

  if (o_pos<o_end) {
    const int l2=std::min(o_end-o_pos,len);
    memcpy(buf,o_pos,l2*sizeof(char));
    o_pos+=l2;
    len-=l2;
    olen+=l2;
  }

  if (len==0) {
    return 0;
  }

  while (1) {
    while (1) {
      const int avail=(end-inpos)*8/bpc;
      if (!avail) {
        break;
      }
      const int l2=std::min(avail,len);
      if (wpos<=l2) {
        idec.decode_u8(bpc,(unsigned char*)buf,inpos,wpos); // updates inpos
        buf+=wpos;
        olen+=wpos;
        len-=wpos;
        wpos=widthp;
        if (len==0) {
          return olen;
        }
        continue;
      }

      const int ilen=l2*bpc/8; // complete input bytes
      const int l3=ilen*8/bpc;

      idec.decode_u8(bpc,(unsigned char*)buf,inpos,l3);
      buf+=l3;
      olen+=l2; // sic!
      len-=l2;  // sic!
      wpos-=l3;

      const int rem=l2-l3;
      if (rem>0) { // only possible with bpc<8 and l2==len
        const int full=8/bpc;
        idec.decode_u8(bpc,(unsigned char*)o_buf,inpos,full);
        wpos-=full;
        memcpy(buf,o_buf,rem*sizeof(char));
        buf+=rem;
        o_end=o_buf+full;
        o_pos=o_end-rem;
        assert(len==0);
      }

      if (len==0) {
        return olen;
      }
    }

    const int clen=end-inpos;
    if (clen>0) {
      memcpy(&block[0],inpos,clen*sizeof(char));
    }
    int res=read_from.read((char *)&block[clen],block.size()-clen);
    if (res==0) {
      if ( (clen>0)|| // (bpc==16)
           (idec.getColorPos()!=0) ) {
        throw UsrError("Premature end");
      }
      // TODO? assert(wpos==widthp);
      return olen;
    }
    idec.invert_1_inplace(&block[clen],res*8);
    inpos=&block.front();
    end=inpos+res;
  }
}
// }}}

long DecodeFilter::FInput::pos() const // {{{
{
  return -1;
}
// }}}

void DecodeFilter::FInput::pos(long pos) // {{{
{
  if (pos!=0) {
    throw std::invalid_argument("Reposition in DecodeFilter_Input is not supported");
  }
  reset();
  read_from.pos(0);
}
// }}}

void DecodeFilter::FInput::reset() // {{{
{
  inpos=end;
  idec.setColorPos(0);
  wpos=widthp;
  o_pos=o_end;
}
// }}}
#undef BLOCK_SIZE


InvertFilter::FInput::FInput(Input &read_from,int width,bool invert) // {{{
  : read_from(read_from),
    widthp(width),wpos(widthp),
    invert(invert)
{
  assert(widthp>0);
}
// }}}

int InvertFilter::FInput::read(char *buf,int len) // {{{
{
  int res=read_from.read(buf,len);

  len=res*8;
  while (wpos<=len) {
    char *tmp=(char *)ImageDecoder::invert_1_inplace((unsigned char *)buf,wpos);
    len-=(tmp-buf)*8;
    buf=tmp;
    wpos=widthp;
  }
  ImageDecoder::invert_1_inplace((unsigned char *)buf,len);
  wpos-=len;

  return res;
}
// }}}

long InvertFilter::FInput::pos() const // {{{
{
  return -1;
}
// }}}

void InvertFilter::FInput::pos(long pos) // {{{
{
  if (pos!=0) {
    throw std::invalid_argument("Reposition in InvertFilter_Input is not supported");
  }
  wpos=widthp;
  read_from.pos(0);
}
// }}}

} // namespace PDFTools
