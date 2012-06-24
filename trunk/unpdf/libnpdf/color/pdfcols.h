#ifndef _PDFCOLS_H
#define _PDFCOLS_H

#include "../io/base.h"
#include "../io/sub.h"
#include "../io/mem.h"
#include "../objs/dict.h"

#include "../pdf/objset.h"
#include "../pdf/objset.h"
#include "../stream/pdfcomp.h"
/*
#include <assert.h>
#include <math.h>
#include "pdfparse.h"
#include "exception.h"
*/

namespace PDFTools {

  class PDF;
  class Name;
  class Array;
  class InStream;
  class Decrypt;
/*
  TODO: The OutPDF contains an array of the used colorspaces.
  output() has to happpen before toObj()

  TODO: or: toObj(Ref ...)
 */
  class ColorSpace : public ObjectSet {
  public:

    virtual Object *toObj(OutPDF &outpdf) const; // the Object-Part

    virtual Ref output(OutPDF &outpdf); // Stream-part (only if needed!)

    virtual int numComps() const; // number of color components
 
    static ColorSpace *parse(PDF &pdf,const Object &obj);
  };

  class SimpleColorSpace : public ColorSpace {
  public:
    enum CName { DeviceGray, DeviceRGB, DeviceCMYK, NUM_NAMES };
    SimpleColorSpace(CName type);

    int numComps() const;

    Object *toObj(OutPDF &outpdf) const;
    static SimpleColorSpace *parse(const char *name);
  private:
    CName type;
    static const char *names[];
  };

  class CieColorSpace : public ColorSpace {
  public:
    enum CName { CalGray, CalRGB, Lab, NUM_NAMES };
    CieColorSpace(CName type,const std::vector<float> &white,
                             const std::vector<float> &black,
                             const std::vector<float> &gamma,
                             const std::vector<float> &matrix,
                             const std::vector<float> &range);

    int numComps() const;

    Object *toObj(OutPDF &outpdf) const;
    static CieColorSpace *parse(PDF &pdf,const char *name,const Array &aval);
    // helper ... also used by ICC
    static std::vector<float> getNums(PDF &pdf,const ObjectPtr &obj,int num);
  private:
    CName type;
    std::vector<float> white,black;
    std::vector<float> gamma; // Gray[1], RGB[3]
    std::vector<float> matrix; // RGB
    std::vector<float> range; // Lab

    static const char *names[];
  };

  class ICCColorSpace : public ColorSpace {
  public:
    enum CName { ICCBased, NUM_NAMES };
    ICCColorSpace(CName type,int numComp,ColorSpace *altcs,
                  const std::vector<float> &range,
                  InStream *metadata,
                  const std::vector<char> &iccdata); // takes >altcs
    ~ICCColorSpace();

    int numComps() const;

    Object *toObj(OutPDF &outpdf) const;
    Ref output(OutPDF &outpdf);
    static ICCColorSpace *parse(PDF &pdf,const char *name,const Array &aval);
  private:
    CName type;
    int numComp;
    ColorSpace *altcs;
    std::vector<float> range;
    OutStream meta;
    std::vector<char> iccdata;
    Ref iccref;

    static const char *names[];
  };

  class PatternColorSpace : public ColorSpace {
  public:
    enum CName { Pattern, NUM_NAMES };
  /*
    // ... TODO

    int numComps() const;

    Object *toObj(OutPDF &outpdf) const;
    Ref output(OutPDF &outpdf);
    static PatternColorSpace *parse(PDF &pdf,const char *name,const Array &aval);
  private:
    InStream stm; // TODO: OutStream
  */

    static const char *names[];
  };

  class IndexedColorSpace : public ColorSpace {
  public:
    enum CName { Indexed, NUM_NAMES };
    IndexedColorSpace(CName type,ColorSpace *base,const std::vector<unsigned char> &palette); // takes
    ~IndexedColorSpace();

    int numComps() const;

    Object *toObj(OutPDF &outpdf) const;
    Ref output(OutPDF &outpdf);
    static IndexedColorSpace *parse(PDF &pdf,const char *name,const Array &aval);
  private:
    CName type;
    ColorSpace *base;
    std::vector<unsigned char> palette;

    static const char *names[];
  };

/*
   [ /Separation name altSpace.colorspace tint.function ]
   [ /DeviceN names.array altSpace.colorspace tint.function ?attrib.dict ]
  class PDFTools::SepDeviceNColorSpace : public ColorSpace {
  public:
    enum CName { Separation, DeviceN, NUM_NAMES };
    ...

    int numComps() const;

    Object *toObj(OutPDF &outpdf) const;
    Ref output(OutPDF &outpdf);
    static CieColorSpace *parse(PDF &pdf,const char *name,const Array &aval);
  private:
    std::vector<Name *> names;
    ColorSpace *altcs;
    std::vector<InStream *> stm; // TODO: Function func;
    Dict attrib; // TODO ?  //ensure no Ref's

    static const char *names[];
  };
*/
};

#endif
