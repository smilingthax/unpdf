#include "pdf.h"
#include <assert.h>
#include "exception.h"

#include "../io/sub.h"

#include "../security/pdfsec.h" // FIXME #include "../security/std.h" // less?

#include "../objs/all.h" // less?

#include "../io/pdfio.h" // FIXME

namespace PDFTools {

PDF::PDF(Input &read_base,int version,int xrefpos) // {{{
  : read_base(read_base),
    version(version),
    xref(false),
    security(NULL)
{
  {
    // read xref and trailer
    read_base.pos(xrefpos);

    ParsingInput pi(read_base);
    if (!xref.parse(pi)) {
      throw UsrError("Could not read xref");
    }

// FIXME: only if hybrid or table:
    // reference says: "trailer precedes startxref" and not "trailer follows xref";
    // but even acrobat seems to just read on instead of skim backwards
    pi.skip(false); // some pdfs require this
    read_trailer(pi);
  }

  // read rootdict
  const Object *pobj;
  if ((pobj=trdict.find("Root"))==NULL) {
    throw UsrError("No Root entry in trailer");
  }
  std::auto_ptr<Object> robj(fetch(dynamic_cast<const Ref &>(*pobj))); // throws if not a Ref (must be indirect)

  Dict *rdict=dynamic_cast<Dict *>(robj.get());
  if (!rdict) {
    throw UsrError("Root catalog is not a dictionary");
  }
  rootdict._move_from(rdict);
  rootdict.ensure(*this,"Type","Catalog");

  // version update?
  pobj=rootdict.find("Version");
  if (pobj) { // version update
    // TODO : may be indirect
    const Name *nval=dynamic_cast<const Name *>(pobj);
    if (!nval) {
      throw UsrError("/Version value is not a Name");
    }
    const char *val=nval->value();
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
  ObjectPtr iobj=trdict.get(*this,"ID");
  if (!iobj.empty()) {
    const Array *aval=dynamic_cast<const Array *>(iobj.get());
    if ( (!aval)||(aval->size()!=2) ) {
      throw UsrError("/ID is not an Array of length 2"); // or not direct
    }
    fileid.first=aval->getString(*this,0);
    fileid.second=aval->getString(*this,1);
  }

  // Encryption
  pobj=trdict.find("Encrypt");
  if (pobj) {
    if (iobj.empty()) {
      throw UsrError("/Encrypt requires /ID");
    }
    encryptref=dynamic_cast<const Ref &>(*pobj); // throws if not a Ref  (not sure ATM if this is really required by spec)
    robj.reset(fetch(encryptref));

    rdict=dynamic_cast<Dict *>(robj.get());
    if (!rdict) {
      throw UsrError("/Encrypt is not a dictionary");
    }

    int filtertype=rdict->getNames(*this,"Filter",NULL,"Standard",NULL);
    assert(filtertype==1);
    //  throw UsrError("Unsupported Security Handler /%s",nval->value());

    security=new StandardSecurityHandler(*this,fileid.first,rdict);
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

Object *PDF::getObject(const Ref &ref) // {{{
{
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

void PDF::read_trailer(ParsingInput &pi) // {{{
{
  if (!pi.next("trailer")) {
    throw UsrError("Could not read trailer");
  }
  pi.skip();
  std::auto_ptr<Dict> tr_dict(Parser::parseDict(pi));
  trdict._move_from(tr_dict.get());
  
  const Object *pobj=trdict.find("Prev");
  while (pobj) {
    const NumInteger *ival=dynamic_cast<const NumInteger*>(pobj);
    if (!ival) {
      throw UsrError("/Prev value is not Integer");
    }
    pi.pos(ival->value());
    if (!xref.parse(pi)) {
      throw UsrError("Could not read previous xref");
    }
    if (!pi.next("trailer")) {
      throw UsrError("Could not read previous trailer");
    }
    pi.skip();
    tr_dict.reset(Parser::parseDict(pi));
    pobj=tr_dict->find("Prev");
  }

  // check size
  pobj=trdict.find("Size");
  if (!pobj) {
    throw UsrError("No /Size in trailer");
  }
  const NumInteger *ival=dynamic_cast<const NumInteger*>(pobj);
  if (!ival) {
    throw UsrError("/Size in trailer is not an Integer");
  }
  if (ival->value()!=(int)xref.size()) {
    throw UsrError("Damaged trailer or xref (size does not match)");
  }
}
// }}}

} // namespace PDFTools
