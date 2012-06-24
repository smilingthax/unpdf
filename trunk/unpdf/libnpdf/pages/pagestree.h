#ifndef UNPDF_PAGES_PAGESTREE_H
#define UNPDF_PAGES_PAGESTREE_H

#include <map>
#include <vector>
#include "../pdf/objset.h"

namespace PDFTools {

  class PDF;
  class OutPDF;
  class Page;
  class Dict;
  class PagesTree : public ObjectSet {
  public:
    PagesTree() {}
    ~PagesTree();

    size_t size() const; // return number of pages
    const Page &operator[](int number) const;
    Ref output(OutPDF &outpdf);

    Page &add();
    void add(OutPDF &outpdf,PDF &srcpdf,int pageno,std::map<Ref,Ref> *donemap);

    void parse(PDF &pdf,const Ref &pgref);

    // convenience
    // ... const Page *operator[](const char *name); // named objects...
  protected:
    struct inherit;
    void parsePagesTree_node(PDF &pdf,const Ref &ref,const Ref *parent,inherit inh);
    void parsePage(PDF &pdf,const Ref &ref,Dict &dict,const inherit &inh); // moves from >dict
  private:
    std::vector<Page *> pages;
  };

} // namespace PDFTools

#endif
