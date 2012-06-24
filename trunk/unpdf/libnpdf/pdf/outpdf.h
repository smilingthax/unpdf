#ifndef UNPDF_PDF_OUTPDF_H
#define UNPDF_PDF_OUTPDF_H

#include <map>
#include "../objs/ref.h"
#include "../objs/dict.h"
#include "../pages/pagestree.h"
#include "../xref/xref.h"

namespace PDFTools {

  class PDF;
  class Input;
  class FILEOutput;
  class Encrypt;
  class OFilter;
  class Array;

  class OutPDF { // TODO: ... for now
  public:
    OutPDF(FILEOutput &write_base);

    Object *copy_from(PDF &inpdf,const Ref &startref,std::map<Ref,Ref> *donemap=NULL);

    Ref newRef();
    Ref outObj(const Object &obj);
    void outObj(const Object &obj,const Ref &ref);

    void finish(const Ref *pgref=NULL);

    // advanced /internally used by OutStream::output
    // returns len; but /Length has to already be set [e.g. as Ref]
    int outStream(const Dict &dict,Input &readfrom,Encrypt *encrypt,OFilter *filter,const Ref &ref);

  //private: 
    FILEOutput &write_base;
  //private: 
    int version;
    XRef xref;
    Dict trdict;
    Dict rootdict;
    PagesTree pages;
  protected:
    void write_header() const;
    void write_trailer(const Ref &pgref);  // xref, trailer
  public:
    // special functions
    void remap_array(PDF &inpdf,Array &aval,std::map<Ref,Ref> *donemap);
    void remap_dict(PDF &inpdf,Dict &dval,std::map<Ref,Ref> *donemap);
  private:
    OutPDF(const OutPDF &);
    const OutPDF &operator=(const OutPDF &);
  };

} // namespace PDFTools

#endif        
