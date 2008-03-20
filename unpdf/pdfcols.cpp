#include <assert.h>
#include "pdfcols.h"
#include "exception.h"
/*
#include <math.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "pdffilter_int.h"
#include "pdfsec.h"
*/

using namespace std;
using namespace PDFTools;

// {{{ PDFTools::ColorSpace
Object *PDFTools::ColorSpace::toObj(OutPDF &outpdf) const
{
  throw UsrError("Empty Colorspace");
}

Ref PDFTools::ColorSpace::print(OutPDF &outpdf)
{
  return Ref();
}

int PDFTools::ColorSpace::numComps() const
{
  return -1;
}

ColorSpace *PDFTools::ColorSpace::parse(PDF &pdf,const Object &obj)
{
  /*auto_ptr*/ ColorSpace *ret=NULL;
  if (const Name *nval=dynamic_cast<const Name *>(&obj)) {
    ret=SimpleColorSpace::parse(nval->value());
    if (!ret) {
      throw UsrError("Unknown colorspace /%s",nval->value());
    }
  } else if (const Array *aval=dynamic_cast<const Array *>(&obj)) {
    if (aval->size()>=1) {
      ObjectPtr nobj=aval->get(pdf,0);
      if (const Name *nval=dynamic_cast<const Name *>(nobj.get())) {
        ( (ret=CieColorSpace::parse(pdf,nval->value(),*aval))!=0 ) ||
        ( (ret=ICCColorSpace::parse(pdf,nval->value(),*aval))!=0 ) ||
//        ( (ret=PatternColorSpace::parse(pdf,nval->value(),*aval))!=0 ) ||
        ( (ret=IndexedColorSpace::parse(pdf,nval->value(),*aval))!=0 ) ;
//        ( (ret=SepDeviceNColorSpace::parse(pdf,nval->value(),*aval))!=0 ) ;
        if (!ret) {
          throw UsrError("Unknown colorspace /%s",nval->value());
        }
      }
    }
  } 
  if (!ret) {
    throw UsrError("Bad ColorSpace");
  }
  return ret;
}
// }}}

// {{{ PDFTools::SimpleColorSpace
// /DeviceRGB  ...
const char *PDFTools::SimpleColorSpace::names[]={"DeviceGray","DeviceRGB","DeviceCMYK"};

PDFTools::SimpleColorSpace::SimpleColorSpace(CName type) : type(type)
{
  if ( (type!=DeviceGray)&&(type!=DeviceRGB)&&(type!=DeviceCMYK) ) {
    throw UsrError("Bad colorspace type");
  } 
}

int PDFTools::SimpleColorSpace::numComps() const
{
  if (type==DeviceGray) {
    return 1;
  } else if (type==DeviceRGB) {
    return 3;
  } else if (type==DeviceCMYK) {
    return 4;
  }
  return -1; // should never happen
}

Object *PDFTools::SimpleColorSpace::toObj(OutPDF &outpdf) const
{
  return new Name(names[type],Name::STATIC);
}

SimpleColorSpace *PDFTools::SimpleColorSpace::parse(const char *name)
{
  int iA;
  for (iA=0;iA<NUM_NAMES;iA++) {
    if (strcmp(name,names[iA])==0) {
      break;
    }
  }
  if (iA==NUM_NAMES) {
    return NULL;
  }
  return new SimpleColorSpace((CName)iA);
}
// }}}

// {{{ PDFTools::CieColorSpace
// [ /CalGray << ... >> ]
const char *PDFTools::CieColorSpace::names[]={"CalGray","CalRGB","Lab"};

PDFTools::CieColorSpace::CieColorSpace(CName type,const vector<float> &white,
                                                  const vector<float> &black,
                                                  const vector<float> &gamma,
                                                  const vector<float> &matrix,
                                                  const vector<float> &range)
                                       : type(type),white(white),black(black),gamma(gamma),
                                         matrix(matrix),range(range)
{
  if ( (type!=CalGray)&&(type!=CalRGB)&&(type!=Lab) ) {
    throw UsrError("Bad colorspace type");
  } 
  if ( (white.size()!=3)||(black.size()!=3) ) {
    throw invalid_argument("Bad white-/blackpoint size");
  }
  if (  ( (type==CalGray)&&(gamma.size()!=1) )||( (type==CalRGB)&&(gamma.size()!=3) )  ) {
    throw invalid_argument("Bad gamma size");
  }
  if ( (type==Lab)&&(range.size()!=4) ) {
    throw invalid_argument("Bad range size");
  }
}

int PDFTools::CieColorSpace::numComps() const
{
  if (type==CalGray) {
    return 1;
  } else if (type==CalRGB) {
    return 3;
  } else if (type==Lab) {
    return 3;
  }
  return -1; // should never happen
}

Object *PDFTools::CieColorSpace::toObj(OutPDF &outpdf) const
{
  auto_ptr<Dict> dict(new Dict);

  dict->add("WhitePoint",Array::getNums(white),true);

  if ( (black[0]!=0)||(black[1]!=0)||(black[2]!=0) ) {
    dict->add("BlackPoint",Array::getNums(black),true);
  }

  if (type==CalGray) {
    if (gamma[0]!=1) {
      dict->add("Gamma",new NumFloat(gamma[0]),true);
    }
  } else if (type==CalRGB) {
    if ( (gamma[0]!=1)||(gamma[1]!=1)||(gamma[2]!=1) ) {
      dict->add("Gamma",Array::getNums(gamma),true);
    }
    if ( (matrix[0]!=1)||(matrix[1]!=0)||(matrix[2]!=0)||
         (matrix[3]!=0)||(matrix[4]!=1)||(matrix[5]!=0)||
         (matrix[6]!=0)||(matrix[7]!=0)||(matrix[8]!=1) ) {
      dict->add("Matrix",Array::getNums(matrix),true);
    }
  } else if (type==Lab) {
    if ( (range[0]!=-100)||(range[1]!=100)||(range[2]!=-100)||(range[3]!=100) ) {
      dict->add("Range",Array::getNums(range),true);
    }
  }

  auto_ptr<Array> ret(new Array);
  ret->add(new Name(names[type],Name::STATIC),true);
  ret->add(dict.release(),true);

  return ret.release();
}

vector<float> PDFTools::CieColorSpace::getNums(PDF &pdf,const ObjectPtr &obj,int num)
{
  const Array *aval=dynamic_cast<const Array *>(obj.get());
  if (!aval) {
    throw UsrError("Numeric array expected");
  }
  return aval->getNums(pdf,num);
}

CieColorSpace *PDFTools::CieColorSpace::parse(PDF &pdf,const char *name,const Array &aval)
{
  if (aval.size()!=2) {
    return NULL;
  }
  int iA;
  for (iA=0;iA<NUM_NAMES;iA++) {
    if (strcmp(name,names[iA])==0) {
      break;
    }
  }
  if (iA==NUM_NAMES) {
    return NULL;
  }
  ObjectPtr dobj=aval.get(pdf,1);
  const Dict *dval=dynamic_cast<const Dict *>(dobj.get());
  if (!dval) {
    throw UsrError("Bad ColorSpace array for /%s",name);
  }

  // get params
  vector<float> white,black;
  white=getNums(pdf,(ObjectPtr_ref)dval->get(pdf,"WhitePoint"),3);
  if (white[1]!=1) {
    throw UsrError("Bad WhitePoint");
  }

  ObjectPtr bobj=dval->get(pdf,"BlackPoint");
  if (!bobj.empty()) {
    black=getNums(pdf,bobj,3);
  } else {
    black.resize(3,0);
  }
 
  vector<float> gamma;
  vector<float> matrix,range;
  if (iA==CalGray) {
    gamma.resize(1,1.0);
    gamma[0]=dval->getNum(pdf,"Gamma",1.0);
  } else if (iA==CalRGB) {
    ObjectPtr cobj=dval->get(pdf,"Gamma");
    if (!cobj.empty()) {
      gamma=getNums(pdf,cobj,3);
    } else {
      gamma.resize(3,1.0);
    }
    ObjectPtr dobj=dval->get(pdf,"Matrix");
    if (!dobj.empty()) {
      matrix=getNums(pdf,dobj,9);
    } else {
      matrix.resize(9,0);
      matrix[0]=matrix[4]=matrix[8]=1;
    }
  } else if (iA==Lab) {
    ObjectPtr cobj=dval->get(pdf,"Range");
    if (!cobj.empty()) {
      range=getNums(pdf,cobj,4);
    } else {
      range.resize(4,-100);
      range[1]=100;
      range[3]=100;
    }
  }
  return new CieColorSpace((CName)iA,white,black,gamma,matrix,range);
}
// }}}

// {{{ PDFTools::ICCColorSpace
// [ /ICCBased  stream ]
// ... more parameters are in the stream dict
const char *PDFTools::ICCColorSpace::names[]={"ICCBased"};

PDFTools::ICCColorSpace::ICCColorSpace(CName type,int numComp,
                                       ColorSpace *altcs,
                                       const vector<float> &range,
                                       InStream *metadata,
                                       const vector<char> &iccdata)
                                       : numComp(numComp),altcs(altcs),range(range),
                                         meta(new MemIOput,true),iccdata(iccdata)
{
  try {
    if (type!=ICCBased) {
      throw UsrError("Bad colorspace type");
    }
    if (numComp==1) {
      if (!altcs) {
         this->altcs=new SimpleColorSpace(SimpleColorSpace::DeviceGray);
      }
    } else if (numComp==3) {
      if (!altcs) {
        this->altcs=new SimpleColorSpace(SimpleColorSpace::DeviceRGB);
      }
    } else if (numComp==4) {
      if (!altcs) {
        this->altcs=new SimpleColorSpace(SimpleColorSpace::DeviceCMYK);
      }
    } else {
      throw UsrError("Bad number of components");
    }
    if ( (!range.empty())&&((int)range.size()!=2*numComp) ) {
      throw UsrError("Invalid Range");
    }
    if (metadata) {
      MemIOput &memout=static_cast<MemIOput &>(meta.getInput());
      copy(memout,const_cast<InStream &>(*metadata).getInput()); // bad
      memout.flush();
    }
  } catch (...) {
    delete altcs;
    throw;
  }
}

PDFTools::ICCColorSpace::~ICCColorSpace()
{
  delete altcs;
}

int PDFTools::ICCColorSpace::numComps() const
{
  return numComp;
}

Object *PDFTools::ICCColorSpace::toObj(OutPDF &outpdf) const
{
  // TODO: if (ref=outpdf.knows(*this)) ...
  if (iccref.ref==0) {
    throw UsrError("OutPDF::print required before toObj"); // TODO: ... toObj(OutPDF &...)
  }
  auto_ptr<Array> ret(new Array);
  ret->add(new Name(names[type]),true);
  ret->add(&iccref,false);

  return ret.release();
}

Ref PDFTools::ICCColorSpace::print(OutPDF &outpdf)
{
  MemInput mip(&iccdata[0],iccdata.size());
  OutStream ostm(&mip,false);

  altcs->print(outpdf);

  ostm.setDict("N",numComp);
//  ostm.setDict("Alternate",altcs->toObj(outpdf),true);
  ostm.unsetDict("Alternate");
  ostm.addDict("Alternate",altcs->toObj(outpdf),true);
  if (!range.empty()) {
    ostm.unsetDict("Range");
  } else {
    int iA;
    for (iA=0;iA<(int)range.size();iA+=2) {
      if ( (range[iA]!=0)||(range[iA+1]!=1) ) {
        break;
      }
    }
    if (iA!=(int)range.size()) {
      ostm.setDict("Range",range);
    } else {
      ostm.unsetDict("Range");
    }
  }
  Ref metaref;
  if (static_cast<MemIOput &>(meta.getInput()).size()) {
    metaref=meta.print(outpdf);
    ostm.setDict("Metadata",metaref);
  } else {
    ostm.unsetDict("Metadata");
  }

  iccref=ostm.print(outpdf);
  return iccref;
}

ICCColorSpace *PDFTools::ICCColorSpace::parse(PDF &pdf,const char *name,const Array &aval)
{
  if (aval.size()!=2) {
    return NULL;
  }
  int iA;
  for (iA=0;iA<NUM_NAMES;iA++) {
    if (strcmp(name,names[iA])==0) {
      break;
    }
  }
  if (iA==NUM_NAMES) {
    return NULL;
  }

  ObjectPtr aobj=aval.get(pdf,1);
  InStream *stm=dynamic_cast<InStream *>(aobj.get());
  if (!stm) {
    throw UsrError("Parse error in /ICCBased");
  }

  int numComp=stm->getDict().getInt(pdf,"N");
  ObjectPtr bobj=stm->getDict().get(pdf,"Alternate");
  auto_ptr<ColorSpace> altcs;
  if (!bobj.empty()) {
    altcs.reset(ColorSpace::parse(pdf,*bobj));
  }

  vector<float> range;
  ObjectPtr cobj=stm->getDict().get(pdf,"Range");
  if (!cobj.empty()) {
    range=CieColorSpace::getNums(pdf,cobj,4);
  }

  InStream *meta=NULL;
  ObjectPtr dobj=stm->getDict().get(pdf,"Metadata");
  if (!dobj.empty()) {
    meta=dynamic_cast<InStream *>(dobj.get());
    if (!meta) {
      throw UsrError("Wrong type of /Metadata");
    }
  }

  MemIOput iccdata;
  {
    InputPtr in=stm->open();
    copy(iccdata,in);
  }

  return new ICCColorSpace((CName)iA,numComp,altcs.release(),range,meta,iccdata.data());
}
// }}}

#if 0
// [ /Pattern ref.stream ]
class PDFTools::PatternColorSpace : public ColorSpace {
public:
  enum CName { Pattern, NUM_NAMES };
/*
  // ... TODO

  int numComps() const;

  Ref print(OutPDF &outpdf);
  static PatternColorSpace *parse(PDF &pdf,const char *name,const Array &aval);
private:
  InStream stm; // TODO: OutStream
*/

  static const char *names[];
};
#endif

// {{{ PDFTools::IndexedColorSpace
// [ /Indexed  base  hival  lookup(string/stream) ]
const char *PDFTools::IndexedColorSpace::names[]={"Indexed"};

PDFTools::IndexedColorSpace::IndexedColorSpace(CName type,ColorSpace *base,const vector<unsigned char> &palette)
                                              : type(type),base(base),palette(palette)
{
  if (type!=Indexed) {
    throw UsrError("Bad colorspace type");
  } 
  if ( (!base)||(dynamic_cast<IndexedColorSpace *>(base))||(dynamic_cast<PatternColorSpace *>(base)) ) {
    throw UsrError("Invalid base Colorspace");
  }
  if ( (palette.size()%base->numComps()!=0)||(palette.size()/base->numComps()>256) ) {
    throw UsrError("Invalid Palette size for /Indexed");
  }
}

PDFTools::IndexedColorSpace::~IndexedColorSpace()
{
  delete base;
}

int PDFTools::IndexedColorSpace::numComps() const
{
  if (type==Indexed) {
    return 1;
  }
  return -1; // should never happen
}

Object *PDFTools::IndexedColorSpace::toObj(OutPDF &outpdf) const
{
  //   [ /Indexed base hival .string/stream ]
  auto_ptr<Array> ret(new Array);
  ret->add(new Name(names[type]),true);
//  ret->add(base,false); //?? TODO ? 
  ret->add(base->toObj(outpdf),true); //?? TODO ? 
  ret->add(new NumInteger(palette.size()/base->numComps()),true);
/*  if (ref.ref!=0) {
    ret->add(&ref,false);
  } else { */
    ret->add(new String((const char *)&palette[0],palette.size(),true));
//  }

  return ret.release();
}

Ref PDFTools::IndexedColorSpace::print(OutPDF &outpdf)
{
#if 0
  MemInput mip(&palette[0],palette.size());
  OutStream ostm(&mip,false);

  ref=ostm.print(outpdf);

  return ref;
#else
  return Ref();
#endif
}

IndexedColorSpace *PDFTools::IndexedColorSpace::parse(PDF &pdf,const char *name,const Array &aval)
{
  if (aval.size()!=4) {
    return NULL;
  }
  int iA;
  for (iA=0;iA<NUM_NAMES;iA++) {
    if (strcmp(name,names[iA])==0) {
      break;
    }
  }
  if (iA==NUM_NAMES) {
    return NULL;
  }

  ObjectPtr aobj=aval.get(pdf,1);
  assert(!aobj.empty());
  auto_ptr<ColorSpace> base(ColorSpace::parse(pdf,*aobj.get()));

  ObjectPtr cobj=aval.get(pdf,2);
  const NumInteger *nival=dynamic_cast<const NumInteger *>(cobj.get());
  if ( (!nival)||(nival->value()<0)||(nival->value()>255) ) {
    throw UsrError("Invalid hival in /Indexed");
  }
  int hival=nival->value();
  const int clen=base->numComps()*(hival+1);

  vector<unsigned char> palette;
  ObjectPtr bobj=aval.get(pdf,3);
  if (const String *sval=dynamic_cast<const String *>(bobj.get())) {
    if ((int)sval->value().size()==clen) {
      palette.resize(clen,0);
      memcpy(&palette[0],&sval->value()[0],clen*sizeof(unsigned char));
    }
  } else if (InStream *stm=dynamic_cast<InStream *>(bobj.get())) {
    InputPtr in=stm->open();
    palette.resize(clen,0);
    int res=in.read((char *)&palette[0],clen);
    if (res!=clen) {
      palette.clear();
    }
  }
  if (palette.empty()) {
    throw UsrError("Parse error in /Indexed");
  }

  return new IndexedColorSpace((CName)iA,base.release(),palette);
}
// }}}

/*
 [ /Separation name altSpace.colorspace tint.function ]
 [ /DeviceN names.array altSpace.colorspace tint.function ?attrib.dict ]
class PDFTools::SepDeviceNColorSpace : public ColorSpace {
public:
  enum CName { Separation, DeviceN, NUM_NAMES };
  ...

  int numComps() const;

  Ref print(OutPDF &outpdf);
  static CieColorSpace *parse(PDF &pdf,const char *name,const Array &aval);
private:
  std::vector<Name *> names;
  ColorSpace *altcs;
  std::vector<InStream *> stm; // TODO: Function func;
  Dict attrib; // TODO ?  //ensure no Ref's

  static const char *names[];
};
*/
