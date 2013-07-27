#include "pagestree.h"
#include <assert.h>
#include "exception.h"
#include "../objs/all.h"
#include "page.h"

#include "../pdf/pdf.h" // FIXME?
#include "../pdf/outpdf.h" // FIXME?

namespace PDFTools {

struct PagesTree::inherit {
  inherit() : mediabox(NULL),cropbox(NULL),rotate(NULL),resources(NULL) {}
  const Array *mediabox,*cropbox;
  const NumInteger *rotate;
  const Dict *resources;
};

PagesTree::~PagesTree() // {{{
{
  for (int iA=0;iA<(int)pages.size();iA++) {
    delete pages[iA];
  }
  pages.clear();
}
// }}}

size_t PagesTree::size() const // {{{
{
  return pages.size();
}
// }}}

const Page &PagesTree::operator[](int number) const // {{{
{
  if ( (number<0)||(number>=(int)pages.size()) ) {
    throw UsrError("Page number %d not found",number);
  }
  return *pages[number];
}
// }}}

Ref PagesTree::output(OutPDF &outpdf) // {{{
{
  Ref ret=outpdf.newRef();

  Array kids;

  for (int iA=0;iA<(int)pages.size();iA++) {
    kids.add(pages[iA]->output(outpdf,ret),true); // TODO no clone?
  }
  // TODO ? optimize with inheritance

  Dict pdict;
  pdict.add("Type","Pages",Name::STATIC);
  pdict.add("Count",(int)pages.size());
  pdict.add("Kids",&kids);

  outpdf.outObj(pdict,ret);

  return ret;
}
// }}}

Page &PagesTree::add() // {{{
{
  pages.push_back(new Page(*this));
  return *pages.back();
}
// }}}

void PagesTree::add(OutPDF &outpdf,PDF &srcpdf,int pageno,std::map<Ref,Ref> *donemap) // {{{
{
  Page &newpage=add();

  newpage.copy_from(outpdf,srcpdf,srcpdf.pages[pageno],donemap);
}
// }}}

// {{{ PagesTree::parse, parsePage, parsePagesTree_node
void PagesTree::parse(PDF &pdf,const Ref &pgref) // {{{
{
  inherit inh;
  parsePagesTree_node(pdf,pgref,NULL,inh);
}
// }}}

void PagesTree::parsePage(PDF &pdf,const Ref &ref,Dict &dict,const inherit &inh) // {{{
{
  if ( (!inh.resources)||(!inh.mediabox) ) {
    throw UsrError("Required /Page entry not specified");
  }
  
  // get mediabox
  Rect medbox(pdf,*inh.mediabox);

  /* TODO get cropbox */

  // get rotation
  int rot=0;
  if (inh.rotate) {
    rot=inh.rotate->value();
    if ( (rot!=0)&&(rot!=90)&&(rot!=180)&&(rot!=270) ) {
      throw UsrError("Bad /Rotation angle");
    }
  }

  // get resources, create page
  pages.push_back(new Page(*this,pdf,ref,dict,*inh.resources));

  pages.back()->setMediaBox(medbox);
 //  pages.back()->setCropBox(cbox);
  pages.back()->setRotation(rot);
}
// }}}

void PagesTree::parsePagesTree_node(PDF &pdf,const Ref &ref,const Ref *parent,inherit inh) // {{{
{
  // get object
  ObjectPtr pobj(pdf.fetch(ref));
  Dict *dict=dynamic_cast<Dict *>(pobj.get());
  if (!dict) {
    throw UsrError("Object is not a Page or Pages dictionary");
  }
  assert(pobj.owns());

  // get type entry
  int pagestype=dict->getNames(pdf,"Type",NULL,"Pages","Page",NULL);

  // check parent link  (prevent loops)
  const Object *ptest=dict->find("Parent");
  if (parent) {
    const Ref *prval=dynamic_cast<const Ref *>(ptest);
    if ( (!prval)||(*prval!=*parent) ) {
      throw UsrError("Bad /Parent in Pages tree");
    }
  } else { // root
    if (ptest) {
      throw UsrError("Pages tree root contains a parent link");
    }
  }

  // do inherit
  ArrayPtr pmb(dict->getArray(pdf,"MediaBox",false));
  if (!pmb.empty()) {
    inh.mediabox=pmb.get();
  }
  ArrayPtr pcb(dict->getArray(pdf,"CropBox",false));
  if (!pmb.empty()) {
    inh.cropbox=pcb.get();
  }
  ObjectPtr prt(dict->get(pdf,"Rotate"));
  if (!prt.empty()) {
    inh.rotate=dynamic_cast<const NumInteger *>(prt.get());
  }
  DictPtr pre(dict->getDict(pdf,"Resources",false));
  if (!pre.empty()) {
    inh.resources=pre.get();
  }

  if (pagestype==1) { // Pages
    // get size
    int size=dict->getInt(pdf,"Count");

    // do kids
    ObjectPtr pkids=dict->get(pdf,"Kids");
    const Array *aval=dynamic_cast<const Array *>(pkids.get());
    if (!aval) {
      throw UsrError("/Kids is not an Array");
    }
    int pgstart=pages.size();
    for (int iA=0;iA<(int)aval->size();iA++) {
      const Ref *rval=dynamic_cast<const Ref *>((*aval)[iA]);
      if (!rval) {
        throw UsrError("/Kids must contain only references");
      }
      parsePagesTree_node(pdf,*rval,&ref,inh);
    }
    if ((int)pages.size()-pgstart!=size) {
      throw UsrError("Wrong /Count in PagesTree");
    }
  } else {
    parsePage(pdf,ref,*dict,inh); // takes!
  }
}
// }}}
// }}}

} // namespace PDFTools
