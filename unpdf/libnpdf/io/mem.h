#ifndef UNPDF_IO_MEM_H
#define UNPDF_IO_MEM_H

#include "base.h"
#include <vector>

namespace PDFTools {

  class MemInput : public Input {
  public:
    MemInput(const char *buf,int len);

    int read(char *buf,int len);
    long pos() const;
    void pos(long pos);

    int size() const;
  private:
    const char *buf;
    int len;
    long cpos;
  };

  class MemIOput : public IOput {
  public:
    MemIOput();

    int read(char *buf,int len);
    long pos() const;
    void pos(long pos);

    void write(const char *buf,int len);

    int size() const;
    const std::vector<char> &data() const { return buf; }
  private:
    std::vector<char> buf;
    long cpos;
  };

} // namespace PDFTools

#endif
