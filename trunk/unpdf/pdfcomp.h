#ifndef _PDFCOMP_H
#define _PDFCOMP_H

#include <string>
#include "pdfbase.h"

namespace PDFTools {
  class PDF;
  class OutPDF;
  class PagesTree;
  class StandardSecurityHandler;
  class OFilter;
  class Rect : public Object { // copyable, "an Array"
  public:
    Rect();
    Rect(float x1,float x2,float y1,float y2);
//    Rect(const Point &p1,const Point &p2);
//    Rect(const Point &p); // (0,0)-p 
    Rect(PDF &pdf,const Array &ary);

    float operator[](int pos) const;
    Array *toArray() const;

    void print(Output &out) const;
  protected:
    float &operator[](int pos);
  private:
    float x1,y1,x2,y2; 
  };

  class ColorSpace;
  class SimpleColorSpace;
  class CieColorSpace;
  class ICCColorSpace;
  class PatternColorSpace;
  class IndexedColorSpace;
  class SepDeviceNColorSpace;

  class ObjectSet { // non-copyable
  public:
    ObjectSet() {}
    virtual ~ObjectSet() {}

    virtual Ref output(OutPDF &outpdf)=0;
  private:
    ObjectSet(const ObjectSet &);
    const ObjectSet &operator=(const ObjectSet &);
  };
  class InStream : public Object {
  public:
    InStream(PDF &pdf,Dict *sdict,SubInput *read_from,const Ref *decryptref=NULL); // moves from >sdict // takes >read_from
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
  class PDF {
  public:
    PDF(Input &read_base,int version,int xrefpos);
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
  protected:
    void read_xref_trailer(ParsingInput &pi);
  private:
    PDF(const PDF &);
    const PDF &operator=(const PDF &);
  };
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
};

#endif
