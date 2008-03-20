#ifndef _PDFIO_H
#define _PDFIO_H

#include <vector>
#include <string>
#include <zlib.h>

namespace PDFTools {
  class Ref;
  class Input { // non-copyable
  public:
    Input() {}
    virtual ~Input() {}

    virtual int read(char *buf,int len)=0; // >=0
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
  class FILEInput : public Input {
  public:
    FILEInput(FILE *f);
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
    FILEOutput(FILE *f);
    FILEOutput(const char *filename);
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
  class InflateInput : public Input {
  public: 
    InflateInput(Input &read_from);
    ~InflateInput();

    int read(char *buf,int len);
    long pos() const;
    void pos(long pos); // not good
  private:
    Input &read_from;
    std::vector<char> buf;
    z_stream zstr;
  };
  class DeflateOutput : public Output {
  public: 
    DeflateOutput(Output &write_to);
    ~DeflateOutput();

    void write(const char *buf,int len);
    void flush();
  private:
    Output &write_to;
    std::vector<char> buf;
    z_stream zstr;
  };
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
    void skip(bool required=true);

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
  class SubInput : public Input {
  public:
    SubInput(Input &read_from,long startpos,long endpos);

    bool empty() const;
    long basepos() const;

    int read(char *buf,int len);
    long pos() const;
    void pos(long pos);
  private:
    Input &read_from;
    long startpos,endpos,cpos;
  };
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
  class Decrypt {
  public:
    virtual ~Decrypt() {}

    virtual void operator()(std::string &dst,const std::string &src) const=0;
    virtual Input *getInput(Input &read_from) const=0;
  };
  class Encrypt {
  public:
    virtual ~Encrypt() {}

    virtual void operator()(std::string &dst,const std::string &src) const=0;
    virtual Output *getOutput(Output &write_to) const=0;
  };

/*
  class OutputStream : public Stream, public Output {
  public:
    OutputStream();

    virtual int vprintf(const char *fmt,va_list ap);
    virtual int write(const char *buf,int len);
    virtual long pos() const;

    virtual Ref setOutput(OutPDFBase *outpdf,Dict *adddict=NULL,Array *filters=NULL); // deletes >addict, >filters
    virtual void finishOutput();

  private:
    OutPDFBase *outpdf;
    Ref lenref;
    int len;
  };
  class OutputFlateStream : public OutputStream {
  public:
    OutputFlateStream();
    ~OutputFlateStream();

    int vprintf(const char *fmt,va_list ap);
    int write(const char *buf,int len);
    //long pos() const; // TODO? not output but input pos?

    Ref setOutput(OutPDFBase *outpdf,Dict *adddict=NULL,Array *filters=NULL); // deletes >addict, >filters
    void finishOutput();

  private:
    int bufsize;
    char *buf;
    z_stream zstr;
    
    OutputFlateStream(const OutputFlateStream&);
    const OutputFlateStream &operator=(const OutputFlateStream&);
  };
  */
  
  void preprint(ParsingInput &in);
  class Object;
  void dump(const Object *obj);

};

#endif
