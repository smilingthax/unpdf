#include "outpdf.h"
#include <assert.h>
#include <memory>
#include "exception.h"

#include "../io/file.h"
#include "../io/ptr.h"
#include "../io/crypt.h"
#include "../objs/all.h"
#include "../util/util.h"
#include "../pdf/pdf.h"

#include "../stream/pdfcomp.h"

// #include "../security/pdfsec.h" // FIXME #include "../security/std.h" // less?

#include "../filter/pdffilter.h" // FIXME
#include "../filter/pdffilter_int.h" // FIXME: needed for output Filter (e.g. /FlateFilter

namespace PDFTools {

OutPDF::OutPDF(FILEOutput &write_base) // {{{
  : write_base(write_base),xref(true)
{
  version=13;
  write_header();
}
// }}}

Ref OutPDF::newRef() // {{{
{
  return xref.newRef();
}
// }}}

Ref OutPDF::outObj(const Object &obj) // {{{
{
  Ref ret=xref.newRef(write_base.sum());

  write_base.printf("%d %d obj\n",ret.ref,ret.gen);
  obj.print(write_base);
  write_base.puts("\nendobj\n");
  return ret;
}
// }}}

void OutPDF::outObj(const Object &obj,const Ref &ref) // {{{
{
  xref.setRef(ref,write_base.sum());

  write_base.printf("%d %d obj\n",ref.ref,ref.gen);
  obj.print(write_base);
  write_base.puts("\nendobj\n");
}
// }}}

int OutPDF::outStream(const Dict &dict,Input &readfrom,Encrypt *encrypt,OFilter *filter,const Ref &ref) // {{{
{
  xref.setRef(ref,write_base.sum());

  write_base.printf("%d %d obj\n",ref.ref,ref.gen);
  dict.print(write_base);
  write_base.puts("\nstream\n");

  long start=write_base.sum();
  if (encrypt) {
    Output *eo=encrypt->getOutput(write_base);
    if (filter) {
      OutputPtr out=filter->open(*eo);
      copy(out,readfrom);
      out.flush();
      delete eo;
    } else {
      OutputPtr out(eo,true);
      copy(out,readfrom);
      out.flush();
    }
  } else if (filter) {
    OutputPtr out=filter->open(write_base);
    copy(out,readfrom);
    out.flush();
  } else {
    copy(write_base,readfrom);
    write_base.flush();
  }
  const int len=write_base.sum()-start;
  write_base.puts("endstream\n"); //  out.puts("\nendstream\n");
  write_base.puts("endobj\n");
  return len;
}
// }}}

void OutPDF::finish(const Ref *pgref) // {{{
{
  if (pgref) {
    write_trailer(*pgref);
  } else {
    // PagesTree
    write_trailer(pages.output(*this));
  }
}
// }}}

/* TODO ? howto generate id before
std::string OutPDF::generate_id_first()
{
  ? TODO
}

// TODO
std::string OutPDF::generate_id_update() // {{{
{
  MD5 hash;
  hash.init();
  hash.update(... current time [also needed for /Info/CreationDate);
  hash.update(... file path);
  hash.update(... file size [current pos???]);
  hash.update(... all values of /Info);
  string ret;
  ret.resize(16);
  hash.finish(&ret[0]);
  return ret;
}
// }}}
*/

// remap/copy references from Array,Dict (modifies >obj)
void OutPDF::remap_array(PDF &inpdf,Array &aval,std::map<Ref,Ref> *donemap) // {{{
{
  for (int iA=0;iA<(int)aval.size();iA++) {
    Object *obj=aval.get(iA);
    if (Ref *ref=dynamic_cast<Ref *>(obj)) {
      aval.set(iA,copy_from(inpdf,*ref,donemap),true);
    } else if (Array *av=dynamic_cast<Array *>(obj)) {
      remap_array(inpdf,*av,donemap);
    } else if (Dict *dv=dynamic_cast<Dict *>(obj)) {
      remap_dict(inpdf,*dv,donemap);
    } // else: no need to modify.
  }
}
// }}}

void OutPDF::remap_dict(PDF &inpdf,Dict &dval,std::map<Ref,Ref> *donemap) // {{{
{
  for (Dict::const_iterator it=dval.begin();it!=dval.end();++it) {
    const Object *obj=it.value();
    if (const Ref *ref=dynamic_cast<const Ref *>(obj)) {
      dval.set(it.key(),copy_from(inpdf,*ref,donemap),true);
    } else if (dynamic_cast<const Array *>(obj)) {
      remap_array(inpdf,*dynamic_cast<Array *>(dval.get(it.key())),donemap);
    } else if (dynamic_cast<const Dict *>(obj)) {
      remap_dict(inpdf,*dynamic_cast<Dict *>(dval.get(it.key())),donemap);
    } // else: no need to modify.
  }
}
// }}}

/* copies the subtree below >startref
 * will return the Ref* to the startref-copy in the new tree
 *  or an Object *, if the reference can be made a direct object
 */
Object *OutPDF::copy_from(PDF &inpdf,const Ref &startref,std::map<Ref,Ref> *donemap) // {{{
{
  if (donemap) {
    std::map<Ref,Ref>::const_iterator it=donemap->find(startref);

    if (it!=donemap->end()) {
      return it->second.clone();
    }
  }

  std::auto_ptr<Object> obj(inpdf.fetch(startref));
  if ( (dynamic_cast<Boolean *>(obj.get()))||
       (dynamic_cast<NumInteger *>(obj.get()))||
       (dynamic_cast<NumFloat *>(obj.get()))||
       (dynamic_cast<String *>(obj.get()))||
       (dynamic_cast<Name *>(obj.get()))||
       (dynamic_cast<Ref *>(obj.get()))||
       (isnull(obj.get())) ) {
    if (1) { // inline
      return obj.release(); // inline these
    } else {
      Ref ret=outObj(*obj);
      if (donemap) {
        donemap->insert(std::make_pair(startref,ret));
      }
      return ret.clone();
    }
  } else if (Array *av=dynamic_cast<Array *>(obj.get())) {
    Ref ret=newRef();
    if (donemap) {
      donemap->insert(std::make_pair(startref,ret));
    }
    remap_array(inpdf,*av,donemap);

    outObj(*obj,ret);
    return ret.clone();
  } else if (Dict *dv=dynamic_cast<Dict *>(obj.get())) {
    Ref ret=newRef();
    if (donemap) {
      donemap->insert(std::make_pair(startref,ret));
    }
    remap_dict(inpdf,*dv,donemap);

    outObj(*obj,ret);
    return ret.clone();
  } else if (InStream *sv=dynamic_cast<InStream *>(obj.get())) {
    Dict &indict=const_cast<Dict &>(sv->getDict()); // BEWARE: we effectively neuter the InStream

    // FIXME: no newRef here ...  -- infinite remap loop is possible
//    const bool rawcopy=false;
    const bool rawcopy=true;

#if 0
// OutStream will disembowel indict
const Object *dparam=indict.find("DecodeParms");
FaxFilter::Params dp;
if (dparam) { // TODO: &&(indict.find("Filter") has "CCITTFaxDecode" for this DecodeParms ...)
  if (const Array *dar=dynamic_cast<const Array *>(dparam)) {
    dparam=(*dar)[0];
  }
  dp=FaxFilter::Params(dynamic_cast<const Dict &>(*dparam)); // FIXME
}
#endif
    // already done in OutStream::output, but we want to avoid remapping any of these
    if (!rawcopy) {
      indict.erase("DecodeParams"); // not used any more (after open(), above)
      indict.erase("Filter");       // and we don't want any (unused) objects copied
    }
    indict.erase("Length");

    remap_dict(inpdf,indict,donemap);

    InputPtr in=sv->open(rawcopy);

    OutStream os(&in,false,&indict);  // TODO?!  InputPtr::operator*() / -> / get()
    if (!rawcopy) {
      FlateFilter::makeOutput(os.ofilter()); // compression?
//      LZWFilter::makeOutput(os.ofilter()); // compression?
#if 0
      if (dparam) {
        FaxFilter::makeOutput(os.ofilter(),dp); // compression?
      }
#endif
    }

    Ref ret=os.output(*this,rawcopy);
    if (donemap) {
      donemap->insert(std::make_pair(startref,ret));
    }
    return ret.clone();
  } else { // unknown type? - just remap
    printf("INFO: unexpected type: %s\n",obj->type());
    Ref ret=outObj(*obj);
    if (donemap) {
      donemap->insert(std::make_pair(startref,ret));
    }
    return ret.clone();
  }
}
// }}}

void PDFTools::OutPDF::write_header() const // {{{
{
  write_base.printf("%%PDF-%d.%d\n",version/10,version%10);
  write_base.puts("%\xe2\xe3\xc2\xd3\n"); // some binary stuff
}
// }}}

void PDFTools::OutPDF::write_trailer(const Ref &pgref) // {{{
{
  // rootdict
  rootdict.erase("Type");
  rootdict.erase("Pages");
  rootdict.add("Type","Catalog",Name::STATIC);
  rootdict.add("Pages",&pgref);
 
  Ref rref=outObj(rootdict);

  // info dict TODO

  // write xref
  long xrpos=write_base.sum();
  xref.print(write_base,false,true);

  // write trailer
  trdict.erase("Size");
  trdict.erase("Root");
  trdict.add("Size",(int)xref.size());
  trdict.add("Root",&rref);

  write_base.puts("trailer\n");
  trdict.print(write_base);
  write_base.put('\n');

  // write the end
  write_base.puts("startxref\n");
  write_base.printf("%ld\n",xrpos);
  write_base.puts("%%EOF\n");
}
// }}}

} // namespace PDFTools
