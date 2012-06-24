#ifndef UNPDF_OBJS_DICT_H
#define UNPDF_OBJS_DICT_H

#include "base.h"
#include <map>
#include <string>
#include "ptr.h"

#include "name.h" // FIXME  Name::FreeT
#include "num.h" // FIXME?  convenience

namespace PDFTools {

  // This class hashes the keys to have faster string compares

  class PDF;
  class Dict : public Object {
  public:
    Dict();
    ~Dict();

    void print(Output &out) const;
    Dict *clone() const;
    void clear();
    bool empty() const;
    size_t size() const;

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
    void add(const char *key,bool value) { add(key,new Boolean(value),true); }
    // these do not preserve Input position: !!!
    void ensure(PDF &pdf,const char *key,const char *name); 
    ObjectPtr get(PDF &pdf,const char *key) const; // notfound: ObjectPtr.empty()
    int getInt(PDF &pdf,const char *key,bool *found=NULL) const; // found==NULL and not found -> throw
    int getInt(PDF &pdf,const char *key,int defval) const;
    float getNum(PDF &pdf,const char *key,float defval) const; // int or float
    bool getBool(PDF &pdf,const char *key,bool defval) const;
    int getNames(PDF &pdf,const char *key,const char *name0,const char *name1,.../*name1,...,NULL*/) const; // if not found: default=0, or throws if (name0==0); always throws on bad name

    std::string getString(PDF &pdf,const char *key) const;
    // when found: ensures direct ... 
    bool getBool_D(const char *key,bool defval) const;
    int getInt_D(const char *key,int defval) const;
    const char *getName_D(const char *key) const; // or NULL
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

} // namespace PDFTools

#endif
