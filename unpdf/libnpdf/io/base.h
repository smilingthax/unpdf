#ifndef UNPDF_IO_BASE_H
#define UNPDF_IO_BASE_H

#include <string>
#include <stdarg.h>

namespace PDFTools {

  class Input { // non-copyable
  public:
    Input() {}
    virtual ~Input() {}

    virtual int read(char *buf,int len)=0; // ret >=0
    virtual std::string gets(int len);
    virtual long pos() const=0;
    virtual void pos(long pos)=0; // pos(0): "reset"
  private:
    Input(const Input &);
    const Input &operator=(const Input &);
  };

  class Output { // non-copyable
  public:
    Output() {}
    virtual ~Output() {}

    virtual void printf(const char *fmt,...);
    virtual void vprintf(const char *fmt,va_list ap); // override if possible
    virtual void write(const char *buf,int len)=0; // has to handle len==-1
    virtual void puts(const char *str);
    virtual void put(const char c); // override if write(,1) would be bad
    virtual void flush()/*=0*/; // flush data, set state as if newly constructed
  private:
    Output(const Output &);
    const Output &operator=(const Output &);
  };

  class IOput : public Input,public Output {
    /*
  public:
    virtual int read(char *buf,int len)=0;
    virtual long pos() const=0;
    virtual void pos(long pos)=0; // pos(0): "reset"

    virtual void write(const char *buf,int len)=0;
    */
  };

} // namespace PDFTools

#endif
