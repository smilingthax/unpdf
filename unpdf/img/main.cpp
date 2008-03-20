#define JPEGLIB_H
#include <leptonica/allheaders.h>
#include <assert.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "exception.h"

using namespace std;
using namespace PDFTools;

class PixOutput : public Output {
public:
  PixOutput(int width,int height,int bpp);
  ~PixOutput();

  void write(const char *buf,int len);
  long pos() const;

  PIX *pix;
private:
  int width,height,bpp;
  int outrow,outcol;
  l_uint32 *data;
  int wpl;

  PixOutput(const PixOutput &);
  const PixOutput &operator=(const PixOutput &);
};

// {{{ PixOutput
PixOutput::PixOutput(int width,int height,int bpp) : width(width),height(height),bpp(bpp),outrow(0),outcol(0)
{
  pix=pixCreate(width,height,bpp);
  if (!pix) {
    throw UsrError("PixCreate failed");
  }
  data=pixGetData(pix);
  wpl=pixGetWpl(pix);
}

PixOutput::~PixOutput()
{
  pixDestroy(&pix);
}

// TODO: format conversations
void PixOutput::write(const char *buf,int len) 
{
  const int bwidth=(width+7)/8;
  for (;outrow<height;outrow++,outcol=0) {
    l_uint32 *line=data+outrow*wpl;
    for (;(outcol<bwidth)&&(len>0);outcol++,len--,buf++) {
      SET_DATA_BYTE(line,outcol,*buf);
    }
    if (!len) {
      break;
    }
  }
  if (len) {
    throw UsrError("PixOutput overflow");
  }
}

long PixOutput::pos() const 
{
  fprintf(stderr,"WARNING: PixOutput::pos() not yet implemented");
  return -1;
}
// }}}

PIX *ourPix(/*const */PIX *pix,int rotate,int maxw,int maxh) // {{{   - rotate and scale 1-bpp to grayscale
{
  PIX *pixd;

  // rotate
  if (rotate==90) {
    pixd=pixRotate90(pix,1);
  } else if (rotate==180) {
    pixd=pixRotate180(NULL,pix);
  } else if (rotate==270) {
    pixd=pixRotate90(pix,-1);
  } else if (rotate==0) {
    pixd=pixClone(pix);
  } else {
    printf("Bad rotate\n");
    return NULL;
  }

  int rwid=pixGetWidth(pixd);
  int rhei=pixGetHeight(pixd);
  float wscale=(float)maxw/rwid,hscale=(float)maxh/rhei;
  float scale;

  if (wscale<hscale) {
    scale=wscale;
  } else {
    scale=hscale;
  }

  // scale
  PIX *pixr;
  pixr=pixScaleToGray(pixd,scale);
  pixDestroy(&pixd);

  return pixr;
}
// }}} 

void do_it(PDF &pdf,int page) // {{{ PNG to stdout of first image (1bpp!) from >page from >pdf
{
  const Dict &rd=pdf.pages[page].getResources();
  ObjectPtr xo=rd.get(pdf,"XObject");
  const Dict *xodict=dynamic_cast<const Dict *>(xo.get());
  if (!xodict) {
    throw UsrError("No XObjects on page %d",page);
  }
  for (Dict::const_iterator it=xodict->begin();it!=xodict->end();++it) {
    ObjectPtr ptr=it.get(pdf);
    InStream *sval=dynamic_cast<InStream *>(ptr.get());
    if ( (sval)&&(sval->getDict().getInt(pdf,"BitsPerComponent",0)==1) ) {
      // TODO: image interface... /Subtype/Image
      int width=sval->getDict().getInt(pdf,"Width");
      int height=sval->getDict().getInt(pdf,"Height");
      int rotate=pdf.pages[page].getRotation();
      PixOutput pxo(width,height,1);
      
      InputPtr sin=sval->open();
      copy(pxo,sin);

      PIX *pix=ourPix(pxo.pix,rotate,800,600);
      pixWriteStream(stdout,pix,IFF_PNG);
      pixDestroy(&pix);
      break;
    }
  }
}
// }}}

int main(int argc, char **argv)
{
  if (argc!=3) {
    fprintf(stderr,"Usage: %s file page\n",argv[0]);
    return 1;
  }
  try {
    FILEInput fi(argv[1]);
    auto_ptr<PDF> pdf=open_pdf(fi);

    do_it(*pdf,atoi(argv[2])-1);
  } catch (exception &e) {
    fprintf(stderr,"Exception: %s\n",e.what());
    return 1;
  }

  return 0;
}
