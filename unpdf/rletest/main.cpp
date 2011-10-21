#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "pdffilter_int.h"
#include "pdfsec.h"
#include "pdfcols.h"
#include "exception.h"

extern "C" {
  #include "gfxfiles.h"
};

using namespace std;
using namespace PDFTools;

// TODO: "cloneable"-object
// SizedInput, SizedOutput

void setFilter(OFilter &filter,int mode,int width,int height,int colors,int bpc)
{
  if (mode==1) {
    AHexFilter::makeOutput(filter);
  } else if (mode==2) {
    A85Filter::makeOutput(filter);
  } else if (mode==3) {
    LZWFilter::makeOutput(filter);
  } else if (mode==4) {
    FlateFilter::makeOutput(filter);
  } else if (mode==5) {
    RLEFilter::makeOutput(filter);
  } else if (mode==6) {
    FaxFilter::Params prm;
    if (bpc!=1) { // hack
      prm.width=((width+7)/8)*8;
    } else {
      prm.width=width;
    }
    prm.invert=true;
    FaxFilter::makeOutput(filter,prm);
  } else if (mode==7) {
    A85Filter::makeOutput(filter);
    FlateFilter::makeOutput(filter);
    AHexFilter::makeOutput(filter);
    FaxFilter::Params prm;
    if (bpc!=1) { // hack
      prm.width=((width+7)/8)*8;
    } else {
      prm.width=width;
    }
    prm.invert=true;
    FaxFilter::makeOutput(filter,prm);
  } else if (mode==8) {
    FlateFilter::Params prm;
    prm.predictor=10;
    prm.width=width;
    prm.bpc=bpc;
    prm.color=colors;
    FlateFilter::makeOutput(filter,prm);
  } else if (mode==9) {
    JpegFilter::Params prm;
//    prm.quality=75;
//    prm.colortransform=...
    prm.width=width;
    prm.height=height;
    prm.color=colors;
// bpc ==8
    JpegFilter::makeOutput(filter,prm);
  }
}

/*
... Object *ptr ...
Image() */

class Image : public OutStream {
public:
  Image(PDF &pdf,InStream *stm); // "takes" 
  Image(PDF &pdf,const InStream &stm); // copies
  ~Image();

  Ref print(OutPDF &outpdf);

protected:
  void parse(PDF &pdf,const InStream &stm);
private:
  int width,height;
  ColorSpace *cs;
  int bpp;
  //  /Intent  name
  bool is_mask; // if (is_mask) -> bpp==1,
  //  /Mask  stream or array  
  //  /Decode  array
  //  /Interpolate  bool(false)
  //  /Alternatives  array
  //  /SMask  stream  -> Image *...
  //  /SMaskInData  int(0)
  //  (/Name) [obsolete]
  //  /StructParent  int
  //  /ID  string [for web capture]
  //  /OPI  dict [alternative dict for OPI; ignored with is_mask]
  //  /Metadata  stream
  //  /OC  dict (names dict)
};

// {{{ Image
Image::Image(PDF &pdf,InStream *stm) : OutStream(stm->release(),true)
{
  parse(pdf,*stm);
  delete stm;
}

Image::Image(PDF &pdf,const InStream &stm) : OutStream(new MemIOput,true)
{
  parse(pdf,stm);
  MemIOput &memout=static_cast<MemIOput &>(getInput());
  copy(memout,const_cast<InStream &>(stm).getInput()); // bad
  memout.flush();
}

Image::~Image()
{
  delete cs;
}

#if 0
Image::getDictInt(const char *key,bool reqd,set<const char *> *found)

void Image::parse(PDF &pdf,const InStream &stm)
{
  const Dict &dict=stm.getDict();
  
  set<const char *> keys;
  for (Dict::const_iterator it=dict.begin();it!=dict.end();++it) {
    keys.insert(it->key());
  }

  dict.ensure("Subtype","Image");
  width=dict.getInt(pdf,"Width"); // reqd
  height=dict.getInt(pdf,"Height"); // reqd

  if (is_mask) {
    cs=dict.get(pdf,"ColorSpace",);
  }
if (cs==Pattern) { throw }  ?? array
  bpp=dict.getInt(pdf,"BitsPerComponent");
  is_mask=dict.getBool("ImageMask",false);
  
  if (is_mask) {
    .
    not
  } else {
  }
  stm->getFilter().isJPX(cs);
  ...

  IFilter *filter=getFilter();
  if ( (filter)&&(filter->isJPX(cs)) ) {
    // unneeded:
    cs =None 
    bpp
    .A.
  } else {
    ObjectPtr fobj=dict.get(pdf,key);
    if (const Name *nval=dynamic_cast<const Name *>(fobj.get())) {
      cs=ColorSpace(*nval);
    }
    .
  }
  // ...

}
#endif

Ref Image::print(OutPDF &outpdf)
{
  setDict("Type","XObject");
  setDict("Subtype","Image");
  setDict("Width",width);
  setDict("Height",height);
  if ( (!is_mask)||(!getFilter().isJPX()) ) {
    assert(cs);
    setDict("ColorSpace",*cs);
    setDict("BitsPerComponent",bpp);
  // } else if (getFilter().isJPX()) { ...
  } else {
    unsetDict("ColorSpace");
    unsetDict("BitsPerComponent");
  }
  // setDict("Intent");
  setDict("ImageMask",is_mask);
#if 0
  if ( (!is_mask)&&(mask) ) {
    setDict("Mask",mask);  // mask is a Ref
  } else {
    unsetDict("Mask");
  }
#endif
//  setDict("Decode", ...

  return OutStream::print(outpdf);
}
// }}}

#if 0
PixOutput::
PixInput::

OutImage, 
class InImage : public ObjectSet {
public:
  Ref print(OutPDF &outpdf);

  
private:
  Input &read_from;
  int width,height;
  int bpp;
  unsigned char 
};

Image::print(OutPDF &outpdf)
{
  .
}
#endif

int read_jpg(const char *filename,char *&buf,int &width,int &height,int &color)
{
  buf=NULL;
  try {
    FILEInput fi(filename);

    JpegFilter::FInput jin(fi);

    jin.get_params(width,height,color);
    buf=(char *)malloc(width*height*color*sizeof(char));
    if (!buf) {
      throw bad_alloc();
    }

    int res=jin.read(buf,width*height*color);
    if (res<width*height*color) {
      fprintf(stderr,"WARNING: read only %d (%d)\n",res,width*height*color);
    }
  } catch (exception &ex) {
    fprintf(stderr,"read_jpg failed: %s\n",ex.what());
    free(buf);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  int decode=0,mode=0;
  vector<const char *> fns;

  for (int iA=1;iA<argc;iA++) {
    if (strcmp(argv[iA],"-d")==0) {
      decode=1;
    } else if (strcmp(argv[iA],"-p")==0) {
      decode=-1;
    } else if (strncmp(argv[iA],"-m",2)==0) {
      mode=atoi(argv[iA]+2);
    } else if (fns.size()<2) {
      fns.push_back(argv[iA]);
    } else {
      fns.clear();
      break;
    }
  }
  if (fns.size()<1) {
    fprintf(stderr,"Usage: %s [-mNUM] [-d|-p] infile [outfile]\n",argv[0]);
    return 1;
  }
  try {
    FILEInput fi(fns[0]);
    FILEOutput fo(fns.size()==2?fns[1]:NULL,stdout);

    if (decode==-1) { // make pdf
      OutPDF outpdf(fo);

int x,y,color=1,bpc=1;

unsigned char *mem=NULL;

int res,bwidth;
if (strcmp(".jpg",fns[0]+strlen(fns[0])-4)==0) {
  char *ms;
  res=read_jpg(fns[0],ms,x,y,color);
  bwidth=x*color;
  bpc=8;
  mem=(unsigned char *)ms;
} else {
  res=read_pbm(fns[0],&mem,&x,&y);
  bwidth=(x+7)/8;
}
assert(res==0);

MemInput memin((char *)mem,bwidth*y);

OutStream imgs(&memin,false);
//imgs.addDict("Type","XObject");
imgs.addDict("Subtype","Image");
imgs.addDict("Width",new NumInteger(x),true);
imgs.addDict("Height",new NumInteger(y),true);
imgs.addDict("BitsPerComponent",new NumInteger(bpc),true);
if (color==1) {
  imgs.addDict("ColorSpace","DeviceGray");
} else if (color==3) {
  imgs.addDict("ColorSpace","DeviceRGB");
} else if (color==4) {
  imgs.addDict("ColorSpace","DeviceCMYK");
}
/*
Array dcar;
dcar.add(new NumInteger(1),true);
dcar.add(new NumInteger(0),true);
imgs.addDict("Decode",&dcar); // TODO? Array("[1 0]");
  imgs.addDict("Decode",Parser::parse("[1 0]"),true);
*/

setFilter(imgs.getFilter(),mode,x,y,color,bpc);
/*
//imgs.addFilter("RunLengthDecode");
// imgs.addFilter(. .); // "RLE","
// ... "Raw"-Mode (jpg/png passthru)
Filter::makeOutput(OFilter *,parame,param,param)
Filter::makeOutput(,Dict params)
*/

      Page &curpg=outpdf.pages.add();

Ref iref=imgs.print(outpdf);
curpg.addResource("XObject","I1",iref.clone(),true); // or: const Ref&, and, .clone() in addResource

int paperx=595,papery=842;
      curpg.setMediaBox(Rect(0,0,paperx,papery));

float scalex=rintf(paperx*1000/(float)x)/1000,scaley=rintf(papery*1000/(float)y)/1000;
if (scalex<scaley) {
  scaley=scalex;
} else {
  scalex=scaley;
}

MemIOput ms;
ms.printf("%.3f 0 0 %.3f 0 0 cm\n"
          "q\n"
          "%d 0 0 %d 0 0 cm\n"
          "/I1 Do\n"
          "Q\n",scalex,scaley,x,y);
OutStream os(&ms,false);
curpg.addContent(os.print(outpdf));

      outpdf.finish();
free(mem);
    } else if (decode==1) {
      if (mode==1) {
        AHexFilter::FInput rfi(fi);
        copy(fo,rfi);
      } else if (mode==2) {
        A85Filter::FInput rfi(fi);
        copy(fo,rfi);
      } else if (mode==3) {
        LZWFilter::FInput rfi(fi);
        copy(fo,rfi);
      } else if (mode==4) {
        FlateFilter::FInput rfi(fi);
        copy(fo,rfi);
      } else if (mode==5) {
        RLEFilter::FInput rfi(fi);
        copy(fo,rfi);
      } else if (mode==9) {
        JpegFilter::FInput rfi(fi);
        copy(fo,rfi);
      } else if (mode==100) {
        Ref r;
        StandardAESDecrypt sec(r,"aaaaaaaaaaaaaaaa");
        auto_ptr<Input> rfi(sec.getInput(fi));
        copy(fo,*rfi);
      } else {
        copy(fo,fi);
      }
      fo.flush();
    } else {
      if (mode==1) {
        AHexFilter::FOutput rfo(fo);
        copy(rfo,fi);
        rfo.flush();
      } else if (mode==2) {
        A85Filter::FOutput rfo(fo);
        copy(rfo,fi);
        rfo.flush();
      } else if (mode==3) {
        LZWFilter::FOutput rfo(fo);
        copy(rfo,fi);
        rfo.flush();
      } else if (mode==4) {
        FlateFilter::FOutput rfo(fo);
        copy(rfo,fi);
        rfo.flush();
      } else if (mode==5) {
        RLEFilter::FOutput rfo(fo);
        copy(rfo,fi);
        rfo.flush();
      } else if (mode==100) {
        Ref r;
        StandardAESEncrypt sec(r,"aaaaaaaaaaaaaaaa");
        auto_ptr<Output> rfo(sec.getOutput(fo));
        copy(*rfo,fi);
        rfo->flush();
      } else {
        copy(fo,fi);
        fo.flush();
      }
    }
  } catch (exception &e) {
    fprintf(stderr,"Exception: %s\n",e.what());
    return 1;
  }

  return 0;
}
