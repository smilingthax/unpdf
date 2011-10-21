#ifndef _PDFSEC_H
#define _PDFSEC_H

#include <string>
#include "pdfcomp.h"

// ATTENTION: {Crypto}Output::flush() will restart crypto!
namespace PDFTools {
  class StandardRC4Crypt : public Decrypt,public Encrypt { // fully symmetric
  public:
    StandardRC4Crypt(const Ref &objref,const std::string &key);
    
    void operator()(std::string &dst,const std::string &src) const;
    Input *getInput(Input &read_from) const;
    Output *getOutput(Output &write_to) const;
  private:
    std::string objkey;
    class RC4Input;
    class RC4Output;
  };
  class StandardAESDecrypt : public Decrypt { // partially symmetric
  public:
    StandardAESDecrypt(const Ref &objref,const std::string &key);
    
    void operator()(std::string &dst,const std::string &src) const;
    Input *getInput(Input &read_from) const;
  private:
    std::string objkey;
    class AESInput;
  };
  class StandardAESEncrypt : public Encrypt { // partially symmetric
  public:
    StandardAESEncrypt(const Ref &objref,const std::string &key,const std::string &iv=std::string());
    
    void operator()(std::string &dst,const std::string &src) const { operator()(dst,src,std::string()); }
    void operator()(std::string &dst,const std::string &src,const std::string &iv) const;
    Output *getOutput(Output &write_to) const;
  private:
    std::string objkey,useiv;
    class AESOutput;
  };

  class StandardSecurityHandler {
  public:
    StandardSecurityHandler(PDF &pdf,const String &firstId,Dict *edict); // moves from >edict

    enum CFMode { StmF=0, StrF, Eff };
    Decrypt *getDecrypt(const Ref &ref,CFMode cfm);

    bool check_userpw(const std::string &user_pw);
    bool check_ownerpw(const std::string &owner_pw);

    // for writing 
    std::pair<std::string,std::string> set_pw(const std::string &owner_pw,const std::string &user_pw); // (owner_hash,user_hash) // TODO
    std::pair<std::string,std::string> set_pw(const std::string &user_pw);
  protected:
    void parse_stdcf(PDF &pdf,const Object *obj);
    static const char pad[];
    std::string compute_key(const std::string &user_pw);
    std::string compute_user_hash(const std::string &ukey);

    std::string compute_owner_key(const std::string &owner_pw);
    std::string encrypt_owner_hash(const std::string &owner_key,const std::string &user_pw);
    std::string decrypt_owner_hash(const std::string &owner_key,const std::string &owner_hash);
  private:
    Dict dict;
    int algo,len;
    int rev,perm;
    std::string docid;
    std::string ownerpw_hash,userpw_hash;
    bool encryptMeta;

    struct CryptFilter {
      CryptFilter() : method(None),event(DocOpen),len(-1) {}
      enum Method { None, V2, AESV2 } method;
      enum AuthEvent { DocOpen, EFOpen } event;
      int len; // len==-1: /Identity
    };
    CryptFilter stdcf,modecf[3];

    std::string ownerpw,userpw; // stores userinput
    std::string filekey; // will be computed; !empty if valid pw known
  };
};

#endif
