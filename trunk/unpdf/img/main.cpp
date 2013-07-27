#define JPEGLIB_H
#include <leptonica/allheaders.h>
#include <assert.h>
#include <stdlib.h>
#include "libnpdf/io/file.h"
#include "libnpdf/io/ptr.h"
#include "libnpdf/pages/page.h"
#include "libnpdf/stream/pdfcomp.h" // FIXME
#include "libnpdf/pdf/pdf.h"
#include "libnpdf/util/util.h"
#include "exception.h"
#include <string.h>

using namespace PDFTools;

class PixOutput : public Output {
public:
  PixOutput(int width,int height,int bpp,bool invert=false);
  ~PixOutput();

  void write(const char *buf,int len);
  long pos() const;

  PIX *pix;
private:
  int width,height,bpp;
  bool invert;
  int outrow,outcol;
  l_uint32 *data;
  int wpl;

  PixOutput(const PixOutput &);
  const PixOutput &operator=(const PixOutput &);
};

// {{{ PixOutput
PixOutput::PixOutput(int width,int height,int bpp,bool invert)
  : width(width),height(height),
    bpp(bpp),invert(invert),
    outrow(0),outcol(0)
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

// TODO: format conversations; currently only for 1bpp
void PixOutput::write(const char *buf,int len)
{
  const int bwidth=(width+7)/8;
  for (;outrow<height;outrow++,outcol=0) {
    l_uint32 *line=data+outrow*wpl;
    if (invert) {
      for (;(outcol<bwidth)&&(len>0);outcol++,len--,buf++) {
        SET_DATA_BYTE(line,outcol,*buf);
      }
    } else {
      for (;(outcol<bwidth)&&(len>0);outcol++,len--,buf++) {
        SET_DATA_BYTE(line,outcol,*buf^0xff); // invert 1bpp, as unpdf internal representation has 0=black
      }
    }
    if (!len) {
      break;
    }
  }
  // TODO: what with one line too much? where does it come from?
//  if (len) {
  if (len>bwidth) {
    throw UsrError("PixOutput overflow by %d",len);
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
  DictPtr xodict=rd.getDict(pdf,"XObject",false);
  if (xodict.empty()) {
    throw UsrError("No XObjects on page %d",page);
  }
  for (Dict::const_iterator it=xodict->begin();it!=xodict->end();++it) {
    ObjectPtr ptr=it.get(pdf);
    InStream *sval=dynamic_cast<InStream *>(ptr.get());
    if (sval) {
      const Dict &sdict=sval->getDict();

      // TODO: image interface... /Subtype/Image
      if (sdict.getInt(pdf,"BitsPerComponent",0)!=1) {
        continue;
      }
      ObjectPtr cso=sdict.get(pdf,"ColorSpace");
      if (cso.empty()) {
        throw UsrError("Missing /ColorSpace");
      } else if (Name *nval=dynamic_cast<Name *>(cso.get())) {
        if (strcmp(nval->value(),"DeviceGray")!=0) {
          continue;
        }
      } else {
        continue;
      }

      int width=sdict.getInt(pdf,"Width");
      int height=sdict.getInt(pdf,"Height");
      int rotate=pdf.pages[page].getRotation();

      bool invert=false;
      ArrayPtr dec=sdict.getArray(pdf,"Decode",false);
      if (!dec.empty()) {
        std::vector<float> decode=dec->getNums(pdf,2);
        if ( (decode[0]==1.0)&&(decode[1]==0.0) ) {
          invert=true;
        } else if ( (decode[0]==0.0)&&(decode[1]==1.0) ) {
          invert=false;
        } else {
          continue;
//        throw UsrError("Unsupported Decode");
        }
      }

      PixOutput pxo(width,height,1,invert);

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
    std::auto_ptr<PDF> pdf=open_pdf(fi);

    do_it(*pdf,atoi(argv[2])-1);
  } catch (std::exception &e) {
    fprintf(stderr,"Exception: %s\n",e.what());
    return 1;
  }

  return 0;
}
