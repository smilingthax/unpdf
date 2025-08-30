#include "page.h"
#include <assert.h>
#include "exception.h"
//#include "../io/base.h"

#include "../pdf/outpdf.h" // FIXME

namespace PDFTools {

Page::Page(PagesTree &parent) // {{{
  : parent(parent),
    readref(0)
{
}
// }}}

Page::Page(PagesTree &parent,PDF &pdf,const Ref &ref,Dict &dict,const Dict &resources) // {{{
  : parent(parent),
    readref(ref.ref,ref.gen)
{
  pdict._move_from(&dict);
  resdict._copy_from(resources);

  const Object *cobj=pdict.find("Contents");
  if (cobj) {
    Object *obj=pdict.get("Contents");
    if (Array *aval=dynamic_cast<Array *>(obj)) {
      content._move_from(aval);
    } else {
      // transfer ownership
      pdict.set("Contents",obj,false);
      content.add(obj,true);
    }
  } // else: empty content

  // remove the things we directly handle
  pdict.erase("Resource");
  pdict.erase("MediaBox");
  pdict.erase("Rotate");
  // pgdict.erase("CropBox"); // leave it for now
  pdict.erase("Contents");
}
// }}}

const Dict &Page::getResources() const // {{{
{
  return resdict;
}
// }}}

void Page::setRotation(int angle) // {{{
{
  if ( (angle!=0)&&(angle!=90)&&(angle!=180)&&(angle!=270) ) {
    throw UsrError("Bad angle");
  }
  rotate=angle;
}
// }}}

int Page::getRotation() const // {{{
{
  return rotate;
}
// }}}

void Page::setMediaBox(const Rect &box) // {{{
{
  mediabox=box;
}
// }}}

void Page::setCropBox(const Rect *box) // {{{
{
  pdict.erase("CropBox");
  if (box) {
    pdict.add("CropBox",box->toArray(),true);
  }
}
// }}}

void Page::addContent(const Ref &ref) // {{{
{
  content.add(ref.clone(),true);
}
// }}}

void Page::addResource(const char *which,const char *name,const Object *obj,bool take) // {{{
{
  // TODO: TODO: redo >resdict
  Dict *dval=NULL;
  Array *aval=NULL;

  if (!resdict.find(which)) {
    if (name) {
      dval=new Dict();
      resdict.add(which,dval,true);
    } else {
      aval=new Array();
      resdict.add(which,aval,true);
    }
  } else {
    Object *obj=resdict.get(which);
    if (name) {
      dval=dynamic_cast<Dict *>(obj);
    } else {
      aval=dynamic_cast<Array *>(obj);
    }
  }
  if (dval) {
    dval->add(name,obj,take);
  } else if (aval) {
    aval->add(obj,take);
  } else {
    throw UsrError("Wrong type");
  }
}
// }}}

Ref *Page::output(OutPDF &outpdf,const Ref &parentref) // {{{
{
  // [- add ProcSet to resdict]  (obsolete... needed for Acrobat 4.0 PS printing...)

  pdict.erase("Type");
  pdict.erase("Parent");
  pdict.erase("MediaBox");
  pdict.erase("Resources");
  pdict.erase("Contents");
  pdict.erase("Rotate");

  pdict.add("Type","Page",Name::STATIC);
  pdict.add("Parent",&parentref);
  pdict.add("MediaBox",&mediabox);
  Ref rdref=outpdf.outObj(resdict);
  pdict.add("Resources",&rdref);
  if (content.size()==1) {
    pdict.add("Contents",content[0]);
  } else if (content.size()>1) {
    pdict.add("Contents",&content);
  }
  if (rotate!=0) {
    pdict.add("Rotate",rotate);
  }

  return outpdf.outObj(pdict).clone();
}
// }}}

const Ref *Page::getReadRef() const // {{{
{
  if (readref.ref==0) {
    return NULL;
  }
  return &readref;
}
// }}}

void Page::copy_from(OutPDF &outpdf,PDF &srcpdf,const Page &page,std::map<Ref,Ref> *donemap) // {{{
{
  pdict._copy_from(page.pdict);

  pdict.erase("Type");
  pdict.erase("Parent");
  pdict.erase("MediaBox");
  pdict.erase("Resources");
  pdict.erase("Contents");
  pdict.erase("Rotate");

  // TODO? better:
  // we need to kill /Beads  (i.e. root/Catalog:  /Threads);
  pdict.erase("B"); // can't sensibly handle Beads -- but they potentially pull in "everything"
  pdict.erase("StructParents"); // no /StructTreeRoot in /Catalog there any more

  outpdf.remap_dict(srcpdf,pdict,donemap);

  mediabox=page.mediabox;
  rotate=page.rotate;

  resdict._copy_from(page.resdict);
  outpdf.remap_dict(srcpdf,resdict,donemap);

  content._copy_from(page.content); // TODO? (now: ref to stream)
  outpdf.remap_array(srcpdf,content,donemap);
}
// }}}

} // namespace PDFTools
