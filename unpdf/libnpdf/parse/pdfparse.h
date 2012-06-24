#ifndef _PDFPARSE_H
#define _PDFPARSE_H

#include <memory>

namespace PDFTools {

  class PDF;
  class Input;
  std::auto_ptr<PDF> open_pdf(Input &fi);

  class SubInput;
  class ParsingInput;

  class Object;
  class Name;
  class Array;
  class Dict;
  class String;
  class Ref;
  class Decrypt;
  namespace Parser {
    Object *parse(const char *string);
    Object *parse(ParsingInput &in,const Decrypt *str_decrypt=NULL); // NULL if no more input
    Object *parseObj(PDF *pdf,SubInput &in,const Ref *ref=NULL); // obj matches >ref? -- and needed for decrypt

    void parseObjNum(ParsingInput &in,long startpos,const Ref *ref=NULL); // obj matches >ref?

    Object *parseNum(ParsingInput &in); // int or float or ref
    String *parseString(ParsingInput &in,const Decrypt *str_decrypt=NULL);
    String *parseHexstring(ParsingInput &in,const Decrypt *str_decrypt=NULL);
    Name *parseName(ParsingInput &in);
    Array *parseArray(ParsingInput &in,const Decrypt *str_decrypt=NULL);
    Dict *parseDict(ParsingInput &in,const Decrypt *str_decrypt=NULL);

    std::pair<int,long> read_pdf(Input &fi); // helper: returns (version,xrefpos)

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
