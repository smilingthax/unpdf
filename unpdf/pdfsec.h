#ifndef _PDFSEC_H
#define _PDFSEC_H

#include <string>
#include "pdfcomp.h"

// ATTENTION: {Crypto}Output::flush() will restart crypto!
namespace PDFTools {
  class StandardRC4Crypt : public Decrypt,public Encrypt { // fully symmetric
  public:
    StandardRC4Crypt(const std::string &objkey);
    
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
    StandardAESDecrypt(const std::string &key);
    
    void operator()(std::string &dst,const std::string &src) const;
    Input *getInput(Input &read_from) const;
  private:
    std::string key;
    class AESInput;
  };
  class StandardAESEncrypt : public Encrypt { // partially symmetric
  public:
    StandardAESEncrypt(const std::string &key,const std::string &iv=std::string());
    
    void operator()(std::string &dst,const std::string &src) const { operator()(dst,src,std::string()); }
    void operator()(std::string &dst,const std::string &src,const std::string &iv) const;
    Output *getOutput(Output &write_to) const;
  private:
    std::string key,useiv;
    class AESOutput;
  };

  class StandardSecurityHandler {
  public:
    StandardSecurityHandler(PDF &pdf,const String &firstId,Dict *edict); // moves from >edict

    enum CFMode { StmF=0, StrF, Eff };
    Decrypt *getDecrypt(const Ref &ref,CFMode cfm,const char *cryptname=NULL);

    bool check_pw(const std::string &pw); // TODO: return which one

// TODO? static:
    // for writing 
    std::pair<std::string,std::string> set_pw(const std::string &owner_pw,const std::string &user_pw); // (owner_hash,user_hash) // TODO
    std::pair<std::string,std::string> set_pw(const std::string &user_pw);
    Dict *set_pw389(const std::string &owner_pw,const std::string &user_pw,int perms,bool encmeta);
  protected:
    static const char pad[];
    std::string get_objkey(const Ref &ref,bool aes);
    std::string compute_key(const std::string &user_pw);
    std::string compute_user_hash(const std::string &ukey);

    std::string compute_owner_key(const std::string &owner_pw);
    std::string encrypt_owner_hash(const std::string &owner_key,const std::string &user_pw);
    std::string decrypt_owner_hash(const std::string &owner_key,const std::string &owner_hash);
    bool check_userpw(const std::string &user_pw);
    bool check_ownerpw(const std::string &owner_pw);
    bool check_ownerpw32a(const std::string &owner_pw);
    bool check_userpw32a(const std::string &user_pw);
    std::pair<std::string,std::string> set_userpw38(const std::string &key,const std::string &user_pw); // (/U,/UE)
    std::pair<std::string,std::string> set_ownerpw39(const std::string &key,const std::string &u_hash,const std::string &owner_pw); // (/O,/OE)
    void validate_perm();
  private:
    Dict dict;
    int algo,len;
    int rev,perm;
    std::string docid;
    std::string ownerpw_hash,userpw_hash;
    bool encryptMeta;
    // new in 1.7 Supp.3
    std::string ownerpw_key,userpw_key,perm_enc;

  public:
    struct CryptFilter {
      CryptFilter() : method(None),event(DocOpen),len(-1) {}
      enum Method { None, V2, AESV2, AESV3 } method;
      enum AuthEvent { DocOpen, EFOpen } event;  // 1.7 Supp.3: only DocOpen
      int len; // len==-1: /Identity, len==0: "default"

      bool parse(PDF &pdf,const Object *obj);
    };
    static void get_cf(PDF &pdf,const Object *cfdict,const char *cryptname,CryptFilter &ret);
  private:
    CryptFilter modecf[3];

    std::string ownerpw,userpw; // stores userinput (ownerpw empty if only userpw known)
    std::string filekey; // will be computed; !empty if valid pw known
  };
};

#endif
