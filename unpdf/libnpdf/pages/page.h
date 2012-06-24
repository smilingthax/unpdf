#ifndef UNPDF_PAGES_PAGE_H
#define UNPDF_PAGES_PAGE_H

#include <map>
#include "rect.h"
#include "../objs/ref.h"
#include "../objs/dict.h"
#include "../objs/array.h"

namespace PDFTools {

  class PDF;
  class OutPDF;
  class PagesTree;

  class Page {
  public:
    const Dict &getResources() const;

    void setRotation(int angle);
    int getRotation() const;

    void setMediaBox(const Rect &box);
    void setCropBox(const Rect *box=NULL); // otherwise ==mediabox 
    // void addResource(Object *obj); // takes; TODO more args
    void addContent(const Ref &ref);
    // advanced
    void addResource(const char *which,const char *name,const Object *obj,bool take=false);

    Ref *output(OutPDF &outpdf,const Ref &parentref);

    const Ref *getReadRef() const; // objno, (if read from)

    void copy_from(OutPDF &outpdf,PDF &srcpdf,const Page &page,std::map<Ref,Ref> *donemap);
  private:
    friend class PagesTree;
    // a page without PagesTree cannot exist
    Page(PagesTree &parent);
    Page(PagesTree &parent,PDF &pdf,const Ref &ref,Dict &dict,const Dict &resources); // moves from >dict
  private:
    PagesTree &parent;
    const Ref readref;
    Dict pdict;

    Rect mediabox;
    Dict resdict;
    Array content;
    int rotate;
  private:
    Page(const Page &);
    const Page &operator=(const Page &);
  };

} // namespace PDFTools

#endif
