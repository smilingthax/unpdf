#ifndef _PDFBASE_H
#define _PDFBASE_H

#include <vector>
#include <map>
#include <string>
#include <zlib.h>

#include "pdfio.h"

namespace PDFTools {
  class XRef;
  class PDF;
  class IFilter;
  class Object { // also the null-object
  public:
    virtual ~Object() {}
    virtual void print(Output &out) const;

    virtual Object *clone() const;
  };
  struct ObjectPtr_ref { // helper,inline
    ObjectPtr_ref(Object *obj,bool take) : ptr(obj),ours(take) {}

    Object *ptr;
    bool ours;
  };
  class ObjectPtr { // an auto_ptr<Object>, that allows non-owned data, inline
  public:
    explicit ObjectPtr(Object *obj=0,bool take=true) : ptr(obj),ours(take) {}
    ~ObjectPtr() {
      if (ours) {
        delete ptr;
      }
    }
    ObjectPtr(ObjectPtr_ref ref) : ptr(ref.ptr),ours(ref.ours) {}

    operator ObjectPtr_ref() { return ObjectPtr_ref(release(),ours); }

    bool empty() const { return (ptr==NULL); }
    bool owns() const { return ours; }
    Object &operator*() const { return *ptr; }
    Object *operator->() const { return ptr; }
    Object *get() const { return ptr; }
    void reset(Object *obj=0,bool take=true);
    void reset(ObjectPtr_ref ref) { reset(ref.ptr,ref.ours); }
    Object *release() { // now your resposibility! check owns()
      Object *ret=ptr;
      ptr=NULL; // TODO: code depends on >ours not being resetted
      return ret;
    }

  private:
    Object *ptr;
    bool ours;

    ObjectPtr(ObjectPtr &optr);
    ObjectPtr &operator=(ObjectPtr &optr);
    ObjectPtr &operator=(ObjectPtr_ref ref);
  };

  class Boolean : public Object {
  public:
    Boolean(bool value);
    void print(Output &out) const;
    Boolean *clone() const { return new Boolean(val); }

    bool value() const { return val; }
  private:
    bool val;
  };
  class NumInteger : public Object {
  public:
    NumInteger(int value=0);
    void print(Output &out) const;
    NumInteger *clone() const { return new NumInteger(val); }

    int value() const { return val; }
  private:
    int val;
  };
  class NumFloat : public Object {
  public:
    NumFloat(float value=0);
    void print(Output &out) const;
    NumFloat *clone() const { return new NumFloat(val); }

    float value() const { return val; }
  private:
    float val;
  };
  class String : public Object {
  public:
    String();
    String(const char *str);
    String(const char *str,int len,bool as_hex=false);
    String(const std::string &str,const Decrypt *decrypt=NULL,bool as_hex=false);
    void print(Output &out) const;
    String *clone() const { return new String(*this); }

    const std::string &value() const { return val; }
  private:
    std::string val; // hehe
    bool as_hex;
  };
  class Name : public Object {
  public:
    enum FreeT { DUP=0, TAKE, STATIC };
    Name(const char *name,FreeT ft=DUP);
    ~Name();
    void print(Output &out) const;
    Name *clone() const { return new Name(val,DUP); } // always copy

    const char *value() const { return val; }
  private:
    const char *val; // to getter + protected
    bool ours;

    Name(const Name&);
    const Name &operator=(const Name&);
  };
  class Ref : public Object {
  public:
    Ref(int ref=0,int gen=0) : ref(ref),gen(gen) {}
    Ref(const Ref &r) : ref(r.ref),gen(r.gen) {}

    void print(Output &out) const;
    Ref *clone() const { return new Ref(ref,gen); }

    bool operator==(const Ref &r) const { return ( (r.ref==ref)&&(r.gen==gen) ); }
    bool operator!=(const Ref &r) const { return ( (r.ref!=ref)||(r.gen!=gen) ); }
    bool operator<(const Ref &r) const { return (ref<r.ref)||( (ref==r.ref)&&(ref<r.gen) ); }

    int ref,gen;
  };
  class Array : public Object {
  public:
    Array();
    ~Array();
  
    void add(const Object *obj,bool take=false);
    void print(Output &out) const;
    Array *clone() const;
    const Object *operator[](int pos) const;
    unsigned int size() const;

    // advanced interface
    Object *get(int pos); // ensures ownership, allow modifications
    void set(int pos,const Object *obj,bool take); // replace(only!) [or change ownership]

    // convenience
    static Array *getNums(const std::vector<float> &nums);
    // these do not preserve Input position: !!!
    ObjectPtr get(PDF &pdf,int pos) const;
    ObjectPtr getTake(PDF &pdf,int pos); // transfer ownership
    std::string getString(PDF &pdf,int pos) const;
    std::vector<float> getNums(PDF &pdf,int num) const;

    void _move_from(Array *from);
    void _copy_from(const Array &from); // "deep copy"
  private:
    struct ArrayType {
      ArrayType(const Object *obj,bool ours);
    
      const Object *obj;
      bool ours;
    };
    typedef std::vector<ArrayType> ArrayVec;
    ArrayVec arvec;

    Array(const Array &);
    const Array &operator=(const Array &);
  };
  class Dict : public Object {
  public:
    Dict();
    ~Dict();

    void print(Output &out) const;
    Dict *clone() const;
    void clear();
    bool empty() const;
    unsigned int size() const;

    void add(const char *key,const Object *obj,bool take=false);
    const Object *find(const char *key) const;
    void erase(const char *key); // Danger: maybe dangling pointers
    class const_iterator;
    const_iterator begin() const;
    const_iterator end() const;

    // advanced interface
    Object *get(const char *key); // ensures ownership, allow modifications
    void set(const char *key,const Object *obj,bool take); // replace(only!)
    void _move_from(Dict *from);
    void _copy_from(const Dict &from); // "deep copy"

    // convenience functions
    void add(const char *key,const char *name,Name::FreeT ft=Name::DUP) { add(key,new Name(name,ft),true); } // "/name"
    void add(const char *key,int value) { add(key,new NumInteger(value),true); }
    void add(const char *key,float value) { add(key,new NumFloat(value),true); }
    // these do not preserve Input position: !!!
    void ensure(PDF &pdf,const char *key,const char *name); 
    ObjectPtr get(PDF &pdf,const char *key) const; // notfound: ObjectPtr.empty()
    int getInt(PDF &pdf,const char *key,bool *found=NULL) const; // found==NULL and not found -> throw
    int getInt(PDF &pdf,const char *key,int defval) const;
    float getNum(PDF &pdf,const char *key,float defval) const; // int or float
    bool getBool(PDF &pdf,const char *key,bool defval) const;
    int getNames(PDF &pdf,const char *key,const char *name0,const char *name1,.../*name1,...,NULL*/) const; // default: 0, throws if (name0==0) or bad name

    std::string getString(PDF &pdf,const char *key) const;
    // when found: ensures direct ... 
    bool getBool_D(const char *key,bool defval) const;
    int getInt_D(const char *key,int defval) const;
  private:
    struct DictType {
      DictType(const char *key,const Object *obj,bool ours);

      char *key;
      const Object *obj;
      bool ours;
    };
    typedef std::multimap<unsigned int,DictType> DictMap;
    DictMap dict;
  private:
    unsigned int hash(const char *key) const;
    DictMap::iterator fnd(const char *key);
    
    Dict(const Dict &);
    const Dict &operator=(const Dict &);
  };
  class Dict::const_iterator {
  public:
    const_iterator(Dict::DictMap::const_iterator it) : it(it) {}

    const char *key() const {
      return it->second.key;
    }
    const Object *value() const {
      return it->second.obj;
    }
    ObjectPtr get(PDF &pdf) const;

    bool operator==(const const_iterator &x) const { return x.it==it; }
    bool operator!=(const const_iterator &x) const { return x.it!=it; }
    const_iterator &operator++() { ++it; return *this; }
  private:
    Dict::DictMap::const_iterator it;
  };

  class XRef {
  public:
    XRef(bool writeable);

    Ref newRef(long pos=-1);
    void setRef(const Ref &ref,long pos);
    void clear();

    bool parse(ParsingInput &pi);
    void print(Output &out,bool as_stream=false);
    size_t size() const;

    long getStart(const Ref &ref) const; // -1 on error
    long getEnd(const Ref &ref) const; // -1 on not_specified
    // convenience 
    //
  public:
    static long readUIntOnly(const char *buf,int len);
  private:
    struct xre_t {
      xre_t() : type(XREF_FREE),off(0),gen(-1) {}
      xre_t(long pos) : type(XREF_USED),off(pos),gen(0) {}
      enum { XREF_FREE, XREF_USED } type; // see also XRef::print
      long off;
      int gen;
    };
    typedef std::vector<xre_t> XRefVec;
    XRefVec xref;
    int xrefpos;
    // ... XRefMap xref_update; bool update_mode;
  protected:
    bool read_xref(ParsingInput &fi,XRefVec &to);
  };

  bool isnull(const Object *obj);
  void fminout(Output &out,float val);
  int copy(Output &out,Input &in,int len=-1);
};

#include "pdfcomp.h"

#endif
