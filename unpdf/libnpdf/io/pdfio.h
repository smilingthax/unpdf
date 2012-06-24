#ifndef _PDFIO_H
#define _PDFIO_H

#include "base.h"
#include <vector>
#include <string>

namespace PDFTools {
  class ParsingInput : public Input {
  public:
    ParsingInput(Input &read_from);

    int read(char *buf,int len);
//    std::string gets(int len); // TODO: we can do better
    long pos() const;
    void pos(long pos);

    // parsing functions, respect comments, etc.
    int pread(char *buf,int len); // ignore comments
    std::pair<const char *,int> pread_to_delim(bool include_ws=false); // only valid till next action with ParsingInput

    void unread(const char *buf,int len);
    bool next(const char c,int prebuffer=1);
    bool next(const char *str,int prebuffer=-1);
    void skip(bool required=true); // skip whitespace

    // convenience functions
    int readUInt();
    int readInt();
    float readFrac(int intPart);
    int read_escape();
  private:
    void buffer(int prebuffer);
    void skip_comment();
  private:
    Input &read_from;
    std::vector<char> prebuf;
    int prepos;
  };

};

#endif
