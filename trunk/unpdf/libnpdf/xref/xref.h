#ifndef UNPDF_XREF_XREF_H
#define UNPDF_XREF_XREF_H

//#include "base.h"
#include <stddef.h>
#include <vector>

namespace PDFTools {

  class Ref;
  class Output;
  class Input;
  class SubInput;
  class ParsingInput;
  class Dict;
  namespace Parser { struct Trailer; }
  class XRef {
  public:
    XRef(bool writeable);

    Ref newRef(long pos=-1);
    void setRef(const Ref &ref,long pos);
    void clear();

    enum ParseMode { BOTH, AS_STREAM, AS_TABLE };
    bool parse(Input &in,Parser::Trailer trailer,Dict *trdict,ParseMode mode=BOTH); // trailer is either part of object, or must be parsed separately (can be NULL)
    void print(Output &out,bool as_stream=false,bool master=true,bool debug=false);
    size_t size() const;

// TODO: only one function to return everything
    long getStart(const Ref &ref) const; // -1 on error
    long getEnd(const Ref &ref) const; // -1 on not_specified
    Ref isObjectStream(const Ref &ref) const; // TODO: better
    // convenience
    //
  public:
    static long readUIntOnly(const char *buf,int len);
    static long readBinary(const char *buf,int len);
  private:
    struct xre_t {
      xre_t() : type(XREF_FREE),off(0),gen(-1),end(-1) {}
      xre_t(long pos) : type(XREF_USED),off(pos),gen(0),end(-1) {}
      enum Type { XREF_FREE=0, XREF_USED=1, XREF_COMPRESSED=2, XREF_UNKNOWN=3 } type; // see also XRef::print
      long off;
      int gen;
      long end; // internally for getEnd
    };
    typedef std::vector<xre_t> XRefVec;
    XRefVec xref;
    std::vector<int> xrefpos;
    // ... XRefMap xref_update; bool update_mode;
  protected:
    bool read_xref(ParsingInput &pi,XRefVec &to);
    Dict *read_xref_stream(SubInput &si,XRefVec &to);
    struct offset_sort;
    void generate_ends();
    void check_trailer(Dict &trdict);
  };

} // namespace PDFTools

#endif
