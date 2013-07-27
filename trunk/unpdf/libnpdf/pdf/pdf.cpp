#include "pdf.h"
#include <assert.h>
#include "exception.h"

#include "../io/sub.h"

#include "../security/pdfsec.h" // FIXME #include "../security/std.h" // less?

#include "../objs/all.h" // less?

#include "../io/pdfio.h" // FIXME
#include "../io/ptr.h"
#include "../stream/pdfcomp.h" // FIXME
#include <stdio.h> // FIXME
//  #include "../util/util.h"
//  #include "../io/file.h"

namespace PDFTools {
//  extern FILEOutput stdfo;

PDF::PDF(Input &read_base,const Parser::Trailer &trailer) // {{{
  : read_base(read_base),
    version(trailer.version),
    xref(false),
    security(NULL)
{
  // read xref and trailer
  if (!xref.parse(read_base,trailer,&trdict)) {
    throw UsrError("Could not read xref");
  }

  // read rootdict
  {
    const Object *pobj=trdict.find("Root");
    if (!pobj) {
      throw UsrError("No Root entry in trailer");
    }

    ObjectPtr robj(fetch(dynamic_cast<const Ref &>(*pobj))); // throws if not a Ref (must be indirect)
    if (Dict *rdict=dynamic_cast<Dict *>(robj.get())) {
      rootdict._move_from(rdict);
    } else {
      throw UsrError("Root catalog is not a dictionary");
    }
  }
  rootdict.ensure(*this,"Type","Catalog");

  // version update?
  Name vers(rootdict.getName(*this,"Version",false));
  if (!vers.empty()) { // version update
    const char *val=vers.value();
    if ( (!isdigit(val[0]))||(val[1]!='.')||(!isdigit(val[2]))||(val[3]) ) {
      throw UsrError("/Version is not a pdf version");
    }
    int newver=(val[0]-'0')*10+(val[2]-'0');
    if (newver<version) {
      throw UsrError("New /Version %d is older than current version %d",newver,version);
    }
    version=newver;
  }

  // get Document ID, not encrypted!
  ArrayPtr iaval=trdict.getArray(*this,"ID",false); // [should probably be direct??]
  if (!iaval.empty()) {
    if (iaval->size()!=2) {
      throw UsrError("/ID has length %d (!=2)",iaval->size());
    }
    fileid.first=iaval->getString(*this,0); // these should be direct
    fileid.second=iaval->getString(*this,1);
  }

  // Encryption
  const Object *pobj=trdict.find("Encrypt");
  if (pobj) {
    if (iaval.empty()) {
      throw UsrError("/Encrypt requires /ID");
    }
    encryptref=dynamic_cast<const Ref &>(*pobj); // throws if not a Ref  (not sure ATM if this is really required by spec)
    ObjectPtr robj(fetch(encryptref));

    Dict *rdict=dynamic_cast<Dict *>(robj.get());
    if (!rdict) {
      throw UsrError("/Encrypt is not a dictionary");
    }

    int filtertype=rdict->getNames(*this,"Filter",NULL,"Standard",NULL);
    assert(filtertype==1);
    //  throw UsrError("Unsupported Security Handler /%s",nval->value());

    security=new StandardSecurityHandler(*this,fileid.first,rdict); // moves from rdict
  }

  try {
    // pages tree
    const Ref *pgref=dynamic_cast<const Ref *>(rootdict.find("Pages"));
    if (!pgref) {
      throw UsrError("No /Pages reference found in root catalog");
    }
    pages.parse(*this,*pgref);
//    dump(pages[0].Content);

  } catch (...) {
    delete security;
    throw;
  }
}
// }}}

PDF::~PDF() // {{{
{
  delete security;
}
// }}}

Decrypt *PDF::getStmDecrypt(const Ref &ref,const char *cryptname) // {{{
{
  if (!security) {
    return NULL;
  }
  return security->getDecrypt(ref,StandardSecurityHandler::StmF,cryptname);
}
// }}}

Decrypt *PDF::getStrDecrypt(const Ref &ref,const char *cryptname) // {{{
{
  if (!security) {
    return NULL;
  }
  return security->getDecrypt(ref,StandardSecurityHandler::StrF,cryptname);
}
// }}}

Decrypt *PDF::getEffDecrypt(const Ref &ref,const char *cryptname) // {{{
{
  if (!security) {
    return NULL;
  }
  return security->getDecrypt(ref,StandardSecurityHandler::Eff,cryptname);
}
// }}}

// TODO elsewhere
std::vector<std::pair<int,int> > loadObjStreamHeader(SubInput &si,int len,int first)
{
  std::vector<std::pair<int,int> > ret;
  ret.reserve(len+1);

  ParsingInput psi(si);

  for (int iA=0;iA<len;iA++) {
    int objno=psi.readUInt();
    psi.skip(true);
    int off=psi.readUInt();
    psi.skip(true);
    ret.push_back(std::make_pair(objno,off+first));
  }

  ret.push_back(std::make_pair(-1,-1));  // TODO? better? elsewhere?

  return ret;
}

// TODO: better Objectstream handling
Object *PDF::getObject(const Ref &ref) // {{{
{
  Ref oref=xref.isObjectStream(ref);
  if (oref.ref) {
    int subobj=oref.gen;
    oref.gen=0;
    std::auto_ptr<Object> ostm(getObject(oref));
    InStream *stm=dynamic_cast<InStream *>(ostm.get());
    if (!stm) {
      throw UsrError("ObjStm expected");
    }
    const Dict &sdict=stm->getDict();
    sdict.ensure(*this,"Type","ObjStm");

    int num=sdict.getInt(*this,"N");
    if ( (subobj<0)||(subobj>num) ) {
      throw UsrError("Bad subobj number for this objstm"); // TODO? how does /Extends work?
    }

    int first=sdict.getInt(*this,"First");

    const Object *eobj=sdict.find("Extends");
    const Ref *extends=dynamic_cast<const Ref *>(eobj);
    if ( (eobj)&&(!extends) ) {
      throw UsrError("Expected /Extends to be a stream reference");
    } else if (extends) {
fprintf(stderr,"WARNING: /Extends not yet supported\n");
    }

    InputPtr in=stm->open();
    SubInput hsi(in,0,first);
    std::vector<std::pair<int,int> > ohdr=loadObjStreamHeader(hsi,num,first); // maybe cache at least this in XRef?

// unread from hsi: first-hsi.pos();
int skip_bytes=ohdr[subobj].second-hsi.pos();
char buf[10];
while (skip_bytes>10) {
  in.read(buf,10);
  skip_bytes-=10;
}
in.read(buf,skip_bytes);

#if 0  // FIXME: Filter does not know, where we are, forbids pos([current position]), but SubInput always does this...
fprintf(stderr,"%d %d\n",in.pos(),ohdr[subobj].second);
    SubInput si(in,ohdr[subobj].second,ohdr[subobj+1].second); // this uses only forward seeking.  // still this needs complete decompression. obj cache might help...

    ParsingInput psi(si);
#endif
    ParsingInput psi(in);

    std::auto_ptr<Decrypt> str_decrypt(getStrDecrypt(ref));
    std::auto_ptr<Object> ret(Parser::parse(psi,str_decrypt.get()));
    if (!ret.get()) { // no more input
      throw UsrError("Could not find complete object here");
    }
    return ret.release();
  }

  int start=xref.getStart(ref);
  if (start==-1)  { // not found: Null-object
    return new Object();
  }
  SubInput si(read_base,start,xref.getEnd(ref));

  if ( (security)&&(ref==encryptref) ) {
    return Parser::parseObj(this,si,NULL); // decryption not applied for direct strings in /Encrypt
  }
  return Parser::parseObj(this,si,&ref);
}
// }}}

Object *PDF::fetch(const Ref &ref) // {{{
{
  std::auto_ptr<Object> obj(getObject(ref));

  Ref *refval;
  while ((refval=dynamic_cast<Ref *>(obj.get()))!=NULL) {
    obj.reset(getObject(*refval));
  }
  return obj.release();
}
// }}}

Object *PDF::fetchP(const Ref &ref) const // {{{
{
  Input &rb=const_cast<Input &>(read_base);
  long spos=rb.pos();
  Object *ret=const_cast<PDF &>(*this).fetch(ref);
  rb.pos(spos);

  return ret;
}
// }}}

ObjectPtr PDF::fetch(const Object *obj) // {{{
{
  if (!obj) {
    throw std::invalid_argument("NULL pointer");
  }
  const Ref *refval=dynamic_cast<const Ref *>(obj);
  if (refval) {
    return ObjectPtr(fetch(*refval),true);
  }
  return ObjectPtr(const_cast<Object *>(obj),false);
}
// }}}

} // namespace PDFTools
