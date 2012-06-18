#ifndef _PDFFILTER_H
#define _PDFFILTER_H

#include <vector>
#include "pdfio.h"
#include "pdfbase.h"

namespace PDFTools {
  class Filter { // non-copyable
  public:
    Filter() {}
    virtual ~Filter() {}
  private:
    Filter(const Filter&);
    const Filter &operator=(const Filter&);
  };
  class IFilter : public Filter {
  public:
    IFilter(PDF &pdf,const Object &filterspec,const Object *decode_params);
    ~IFilter();

    InputPtr open(Input *read_from,bool take); // TODO ... Decrypt * // StandardSecurityHandler * ... (key) ...
// TODO(?)  InputPtr open(...,int only_filters=-1)

//    ...
    int hasBpp() const;
    bool isJPX() const;
    bool isJPX(ColorSpace &cs) const;
    const char *hasCrypt() const;
  protected:
    void init(const Array &filterspec,const Array &decode_params,Input &read_from);
    static void lateCloseFunc(Input *in,void *user);
  private:
    InputPtr latein;
    std::vector<Input *> filter_chain;
    Array filter;
    Array params;
    const char *cryptname; // around as long as params is around
  };
  class OFilter : public Filter {
  public:
    OFilter();
    ~OFilter();

    const Object *getFilter() const;
    const Object *getParams() const;
    OutputPtr open(Output &write_to);

    bool isJPX() const;
    // interface for Filter::makeOutput(..)
    Output &getOutput();
    void addFilter(const char *name,Dict *params,Output *out); // takes >param and >out, >name must be static
  private:
    OutputPtr lateout;
    std::vector<Output *> filter_chain;
    Array filter;
    Array params;
  };
};

#endif
