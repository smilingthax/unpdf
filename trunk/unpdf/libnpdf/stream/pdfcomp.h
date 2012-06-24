#ifndef _PDFCOMP_H
#define _PDFCOMP_H

#include <string>
#include <vector>
#include "../objs/base.h"
#include "../objs/dict.h"

#include "../pdf/objset.h" // FIXME

namespace PDFTools {
  class PDF;
  class Input;
  class SubInput;
  class OutPDF;
  class StandardSecurityHandler;
  class IFilter;
  class OFilter;
  class Decrypt;
  class Encrypt;
  class InputPtr;

  class InStream : public Object {
  public:
    InStream(PDF *pdf,Dict *sdict,SubInput *read_from,const Ref *decryptref=NULL); // moves from >sdict // takes >read_from
    ~InStream();

    const Dict &getDict() const;
    InputPtr open(bool raw=false);

    void print(Output &out) const;
    void write(const char *filename) const;

    // advanced interface
    SubInput *release(); // now SubInput is YOURS; better you destroy stm now.
    SubInput &getInput() { return *readfrom; }
    IFilter *getFilter() { return filter; }
  private:
    Dict dict;
    SubInput *readfrom;
    Decrypt *decrypt;
    IFilter *filter;

    InStream(const InStream &stream);
    const InStream &operator=(const InStream &);
  };

  class OutStream : public ObjectSet {
  public:
    OutStream(Input *read_from,bool take,Dict *sdict=NULL); // moves from >sdict
    ~OutStream();

    Ref output(OutPDF &outpdf,bool raw);
    Ref output(OutPDF &outpdf) { return output(outpdf,false); }

    void addDict(const char *key,const Object *obj,bool take=false);
    OFilter &ofilter(); // also for modification
    // convenience
    void addDict(const char *key,const char *name); // !! static

    void setDict(const char *key,const char *nval);
    void setDict(const char *key,int ival);
    void setDict(const char *key,const Object &obj);
    void setDict(const char *key,const std::vector<float> &nums);
    void unsetDict(const char *key);

    // advanced interface
    Input &getInput() { return *readfrom; }
    bool isJPX() const;
  private:
    Dict dict;
    Input *readfrom;
    bool ours;
    Encrypt *encrypt;
    OFilter *filter;
  };
};

#endif
