#ifndef UNPDF_PDF_OBJSET_H
#define UNPDF_PDF_OBJSET_H

#include "../objs/ref.h"

namespace PDFTools {

  class OutPDF;
  class ObjectSet { // non-copyable
  public:
    ObjectSet() {}
    virtual ~ObjectSet() {}

    virtual Ref output(OutPDF &outpdf)=0;
  private:
    ObjectSet(const ObjectSet &);
    const ObjectSet &operator=(const ObjectSet &);
  };

} // namespace PDFTools

#endif
