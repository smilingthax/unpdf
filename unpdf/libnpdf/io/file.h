#ifndef UNPDF_IO_FILE_H
#define UNPDF_IO_FILE_H

#include "base.h"
#include <stdio.h>

namespace PDFTools {

  class FILEInput : public Input {
  public:
    FILEInput(FILE *f,bool take=false);
    FILEInput(const char *filename);
    ~FILEInput();

    int read(char *buf,int len); // >=0, errors are thrown, eof...0
    long pos() const;
    void pos(long pos);
  private:
    FILE *f;
    bool ourclose;
  };

  class FILEOutput : public Output {
  public:
    FILEOutput(FILE *f,bool take=false);
    FILEOutput(const char *filename);
    FILEOutput(const char *filename,FILE *_f); // e.g. (fn/NULL,stdout)
    ~FILEOutput();

    void vprintf(const char *fmt,va_list ap);
    void write(const char *buf,int len);
    long sum() const;
    void flush();
  private:
    FILE *f;
    bool ourclose;
    long sumout;
  };

} // namespace PDFTools

#endif
