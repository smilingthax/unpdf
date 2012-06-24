#ifndef UNPDF_IO_PTR_H
#define UNPDF_IO_PTR_H

#include "base.h"

namespace PDFTools {

  struct InputPtr_ref { // helper, inline
    InputPtr_ref(Input *in,bool take,void (*closefn)(Input *,void *),void *user)
      : in(in),ours(take),closefn(closefn),user(user) {}

    Input *in;
    bool ours;
    void (*closefn)(Input *,void *);
    void *user;
  };

  class InputPtr : public Input { // inline
    typedef void (*CLOSEFN)(Input *,void *);
  public:
    explicit InputPtr(Input &input) : in(&input),ours(false),closefn(NULL),user(NULL) {}
    InputPtr(Input *input,bool take)
        : in(input),ours(take),closefn(NULL),user(NULL) {}
    InputPtr(Input *input,bool take,CLOSEFN closefn,void *user=NULL)
        : in(input),ours(take),closefn(closefn),user(user) {}
    ~InputPtr() {
      if (closefn) {
        closefn(in,user);
      }
      if (ours) {
        delete in;
      }
    }
    InputPtr(InputPtr_ref ref) 
        : Input(),in(ref.in),ours(ref.ours),closefn(ref.closefn),user(ref.user) {}
    operator InputPtr_ref() { // {{{
      // store
      Input *inp=in; 
      bool take=ours;
      CLOSEFN cfn=closefn;
      void *usr=user;
      // reset
      in=NULL;
      ours=false;
      closefn=NULL;
      user=NULL;

      return InputPtr_ref(inp,take,cfn,usr); 
    } // }}}

    int read(char *buf,int len) { return in->read(buf,len); }
    std::string gets(int len) { return in->gets(len); }
    long pos() const { return in->pos(); }
    void pos(long pos) { in->pos(pos); }

    bool empty() const { return (in==NULL); }
    void reset(Input *newin,bool take=false); // beware!
  private:
    Input *in;
    bool ours;
    CLOSEFN closefn;
    void *user;

    InputPtr(InputPtr &ptr);
  };

  struct OutputPtr_ref { // helper, inline
    OutputPtr_ref(Output *out,bool take,void (*closefn)(Output *,void *),void *user)
      : out(out),ours(take),closefn(closefn),user(user) {}

    Output *out;
    bool ours;
    void (*closefn)(Output *,void *);
    void *user;
  };

  class OutputPtr : public Output { // inline
    typedef void (*CLOSEFN)(Output *,void *);
  public:
    explicit OutputPtr(Output &output) : out(&output),ours(false),closefn(NULL),user(NULL) {}
    OutputPtr(Output *output,bool take)
        : out(output),ours(take),closefn(NULL),user(NULL) {}
    OutputPtr(Output *output,bool take,CLOSEFN closefn,void *user=NULL)
        : out(output),ours(take),closefn(closefn),user(user) {}
    ~OutputPtr() {
      if (closefn) {
        closefn(out,user);
      }
      if (ours) {
        delete out;
      }
    }
    OutputPtr(OutputPtr_ref ref) 
        : Output(),out(ref.out),ours(ref.ours),closefn(ref.closefn),user(ref.user) {}
    operator OutputPtr_ref() { // {{{
      // store
      Output *outp=out; 
      bool take=ours;
      CLOSEFN cfn=closefn;
      void *usr=user;
      // reset
      out=NULL;
      ours=false;
      closefn=NULL;
      user=NULL;

      return OutputPtr_ref(outp,take,cfn,usr); 
    } // }}}

    void vprintf(const char *fmt,va_list ap) { out->vprintf(fmt,ap); }
    void write(const char *buf,int len) { out->write(buf,len); }
    void puts(const char *str) { out->puts(str); }
    void put(const char c) { out->put(c); }
    void flush() { out->flush(); }

    bool empty() const { return (out==NULL); }
    void reset(Output *newout,bool take=false); // beware!
  private:
    Output *out;
    bool ours;
    CLOSEFN closefn;
    void *user;

    OutputPtr(OutputPtr &ptr);
  };

} // namespace PDFTools

#endif
