#ifndef UNPDF_PDF_PDF_H
#define UNPDF_PDF_PDF_H

#include "../objs/ref.h"
#include "../objs/ptr.h"
#include "../objs/dict.h"
#include "../pages/pagestree.h"
#include "../xref/xref.h"

#include "../parse/pdfparse.h"

namespace PDFTools {

  class Input;
  class StandardSecurityHandler;
  class PDF {
  public:
    PDF(Input &read_base,const Parser::Trailer &trailer);
    ~PDF();

    ObjectPtr fetch(const Object *obj); // throws if NULL!; if >obj a reference: fetch, otherwise return ObjectPtr(obj,false) TODO? bad const_cast
    Object *fetch(const Ref &ref); // resolve all References
    Object *fetchP(const Ref &ref) const; // resolve ..., and restore Input position
    Object *getObject(const Ref &ref); // none: Null-object

    Decrypt *getStmDecrypt(const Ref &ref,const char *cryptname=NULL);
    Decrypt *getStrDecrypt(const Ref &ref,const char *cryptname=NULL);
    Decrypt *getEffDecrypt(const Ref &ref,const char *cryptname=NULL);

  //private:
    Input &read_base;
  //private:
    int version;
    XRef xref;
    Dict trdict;
    Dict rootdict;
    PagesTree pages;
  private:
    std::pair<std::string,std::string> fileid;
    StandardSecurityHandler *security; // for now...
    Ref encryptref;
  private:
    PDF(const PDF &);
    const PDF &operator=(const PDF &);
  };

} // namespace PDFTools

#endif
