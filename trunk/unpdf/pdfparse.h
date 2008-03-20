#ifndef _PDFPARSE_H
#define _PDFPARSE_H

#include "pdfio.h"
#include "pdfbase.h"

namespace PDFTools {
  std::auto_ptr<PDF> open_pdf(Input &fi);

  namespace Parser {
    Object *parse(const char *string);
    Object *parse(ParsingInput &in,const Decrypt *stm_decrypt=NULL,const Decrypt *str_decrypt=NULL); // NULL if no more input
    Object *parseObj(PDF &pdf,SubInput &in,const Ref *ref=NULL); // obj matches >ref?

    Object *parseNum(ParsingInput &in); // int or float or ref
    String *parseString(ParsingInput &in,const Decrypt *str_decrypt=NULL);
    String *parseHexstring(ParsingInput &in,const Decrypt *str_decrypt=NULL);
    Name *parseName(ParsingInput &in);
    Array *parseArray(ParsingInput &in,const Decrypt *stm_decrypt=NULL,const Decrypt *str_decrypt=NULL);
    Dict *parseDict(ParsingInput &in,const Decrypt *stm_decrypt=NULL,const Decrypt *str_decrypt=NULL);

    std::pair<int,long> read_pdf(Input &fi); // helper: returns (version,xrefpos)

    extern const char charType[256];
    inline bool is_delim(unsigned char c) { // space or special
      return (charType[c]&3);
    }
    inline bool is_space(unsigned char c) {
      return (charType[c]&1);
    }
    inline int skip_eol(const char *buf) {
      if (*buf=='\n') {
        return 1;
      } else if (*buf=='\r') {
        ++buf;
        if (*buf=='\n') {
          return 2;
        }
        return 1;
      }
      return 0;
    }
    inline int one_hex(unsigned char c) { // assert(isxdigit(c))
      if (c<='9') {
        return c-'0';
      } else {
        return (c|0x20)-'a'+10;
      }
    }
  };
};

#endif
