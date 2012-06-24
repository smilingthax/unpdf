#ifndef UNPDF_XREF_XREF_H
#define UNPDF_XREF_XREF_H

//#include "base.h"
#include <stddef.h>
#include <vector>

namespace PDFTools {

  class Ref;
  class Output;
  class ParsingInput;
  class XRef {
  public:
    XRef(bool writeable);

    Ref newRef(long pos=-1);
    void setRef(const Ref &ref,long pos);
    void clear();

    enum ParseMode { BOTH, AS_STREAM, AS_TABLE };
    bool parse(ParsingInput &pi,ParseMode mode=BOTH);
    void print(Output &out,bool as_stream=false,bool master=true);
    size_t size() const;

    long getStart(const Ref &ref) const; // -1 on error
    long getEnd(const Ref &ref) const; // -1 on not_specified
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
    bool read_xref(ParsingInput &fi,XRefVec &to,bool ignore_stream=false);
    bool read_xref_stream(ParsingInput &fi,XRefVec &to);
    struct offset_sort;
    void generate_ends();
  };

} // namespace PDFTools

#endif
