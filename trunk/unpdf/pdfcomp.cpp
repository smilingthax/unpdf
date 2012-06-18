#include <assert.h>
#include <memory>
#include "exception.h"
#include "pdfcomp.h"

#include <typeinfo>

using namespace std;
using namespace PDFTools;

// {{{ PDFTools::Rect
PDFTools::Rect::Rect() : x1(0),y1(0),x2(0),y2(0)
{
}

PDFTools::Rect::Rect(float x1,float y1,float x2,float y2) : x1(x1),y1(y1),x2(x2),y2(y2)
{
}

#if 0
PDFTools::Rect::Rect(const Point &p1,const Point &p2) : x1(p1.x),y1(p1.y),x2(p2.x),y2(p2.y)
{
}

PDFTools::Rect::Rect(const Point &p) : x1(0),y1(0),x2(p.x),y2(p.y)
{
}
#endif

PDFTools::Rect::Rect(PDF &pdf,const Array &ary)
{
  if (ary.size()!=4) {
    throw UsrError("Array is not a Rectangle (wrong size)");
  }

  for (int iA=0;iA<4;iA++) {
    ObjectPtr ptr(ary.get(pdf,iA));
    const NumFloat *fval=dynamic_cast<const NumFloat *>(ptr.get());
    const NumInteger *ival=dynamic_cast<const NumInteger *>(ptr.get());
    if (fval) {
      operator[](iA)=fval->value();
    } else if (ival) {
      operator[](iA)=(float)ival->value();
    } else {
      throw UsrError("Array is not a Rectangle (wrong types)");
    }
  }
}

float PDFTools::Rect::operator[](int pos) const
{
  if (pos==0) {
    return x1;
  } else if (pos==1) {
    return y1;
  } else if (pos==2) {
    return x2;
  } else if (pos==3) {
    return y2;
  } else {
    throw UsrError("Bad index for rect: %d",pos);
  }
}

float &PDFTools::Rect::operator[](int pos)
{
  if (pos==0) {
    return x1;
  } else if (pos==1) {
    return y1;
  } else if (pos==2) {
    return x2;
  } else if (pos==3) {
    return y2;
  } else {
    throw UsrError("Bad index for rect: %d",pos);
  }
}

Array *PDFTools::Rect::toArray() const
{
  auto_ptr<Array> ret(new Array);

  for (int iA=0;iA<4;iA++) {
    ret->add(new NumFloat((*this)[iA]),true);
  }
  return ret.release();
}

void PDFTools::Rect::print(Output &out) const
{
  out.puts("[");
  fminout(out,x1);
  out.puts(" ");
  fminout(out,y1);
  out.puts(" ");
  fminout(out,x2);
  out.puts(" ");
  fminout(out,y2);
  out.puts("]");
//  out.printf("[%f %f %f %f]",x1,y1,x2,y2);
}
// }}}

// {{{ PDFTools::Page
PDFTools::Page::Page(PagesTree &parent) : parent(parent),readref(0)
{
}

PDFTools::Page::Page(PagesTree &parent,PDF &pdf,const Ref &ref,Dict &dict,const Dict &resources) : parent(parent),readref(ref.ref,ref.gen)
{
  pdict._move_from(&dict);
  resdict._copy_from(resources);
  Object *obj=pdict.get("Contents");
  if (Array *aval=dynamic_cast<Array *>(obj)) {
    content._move_from(aval);
  } else if (obj) {
    // transfer ownership
    pdict.set("Contents",obj,false);
    content.add(obj,true);
  } else {
    assert(!pdict.find("Contents"));
  }

  // remove the things we directly handle
  pdict.erase("Resource");
  pdict.erase("MediaBox");
  pdict.erase("Rotate");
  // pgdict.erase("CropBox"); // leave it for now
  pdict.erase("Contents");
}

const Dict &PDFTools::Page::getResources() const
{
  return resdict;
}

void PDFTools::Page::setRotation(int angle) 
{
  if ( (angle!=0)&&(angle!=90)&&(angle!=180)&&(angle!=270) ) {
    throw UsrError("Bad angle");
  }
  rotate=angle;
}

int PDFTools::Page::getRotation() const
{
  return rotate;
}

void PDFTools::Page::setMediaBox(const Rect &box)
{
  mediabox=box;
}

void PDFTools::Page::setCropBox(const Rect *box)
{
  pdict.erase("CropBox");
  if (box) {
    pdict.add("CropBox",box->toArray(),true);
  }
}

void PDFTools::Page::addContent(const Ref &ref)
{
  content.add(ref.clone(),true);
}

void PDFTools::Page::addResource(const char *which,const char *name,const Object *obj,bool take)
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

Ref *PDFTools::Page::output(OutPDF &outpdf,const Ref &parentref)
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

const Ref *PDFTools::Page::getReadRef() const
{
  if (readref.ref==0) {
    return NULL;
  }
  return &readref;
}

void PDFTools::Page::copy_from(OutPDF &outpdf,PDF &srcpdf,const Page &page,map<Ref,Ref> *donemap)
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

// {{{ PDFTools::PagesTree
struct PDFTools::PagesTree::inherit { 
  inherit() : mediabox(NULL),cropbox(NULL),rotate(NULL),resources(NULL) {}
  const Array *mediabox,*cropbox;
  const NumInteger *rotate;
  const Dict *resources;
};

PDFTools::PagesTree::~PagesTree()
{
  for (int iA=0;iA<(int)pages.size();iA++) {
    delete pages[iA];
  }
  pages.clear();
}

size_t PDFTools::PagesTree::size() const 
{
  return pages.size();
}

const Page &PDFTools::PagesTree::operator[](int number) const
{
  if ( (number<0)||(number>=(int)pages.size()) ) {
    throw UsrError("Page number %d not found",number);
  }
  return *pages[number];
}

Ref PDFTools::PagesTree::output(OutPDF &outpdf)
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

Page &PDFTools::PagesTree::add()
{
  pages.push_back(new Page(*this));
  return *pages.back();
}

void PDFTools::PagesTree::add(OutPDF &outpdf,PDF &srcpdf,int pageno,map<Ref,Ref> *donemap)
{
  Page &newpage=add();

  newpage.copy_from(outpdf,srcpdf,srcpdf.pages[pageno],donemap);
}

// {{{ PagesTree::parse, parsePage, parsePagesTree_node
void PDFTools::PagesTree::parse(PDF &pdf,const Ref &pgref)
{
  inherit inh;
  parsePagesTree_node(pdf,pgref,NULL,inh);
}

void PDFTools::PagesTree::parsePage(PDF &pdf,const Ref &ref,Dict &dict,const inherit &inh)
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

void PDFTools::PagesTree::parsePagesTree_node(PDF &pdf,const Ref &ref,const Ref *parent,inherit inh)
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
  ObjectPtr pmb(dict->get(pdf,"MediaBox"));
  if (!pmb.empty()) {
    inh.mediabox=dynamic_cast<const Array *>(pmb.get());
  }
  ObjectPtr pcb(dict->get(pdf,"CropBox"));
  if (!pcb.empty()) {
    inh.cropbox=dynamic_cast<const Array *>(pcb.get());
  }
  ObjectPtr prt(dict->get(pdf,"Rotate"));
  if (!prt.empty()) {
    inh.rotate=dynamic_cast<const NumInteger *>(prt.get());
  }
  ObjectPtr pre(dict->get(pdf,"Resources"));
  if (!pre.empty()) {
    inh.resources=dynamic_cast<const Dict *>(pre.get());
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

#include "pdffilter.h"
#include <stdio.h>

extern FILEOutput stdfo;

// TODO: InStream(...,SubInput *read_from,...)  // where >read_from is a separate FILEInput ? open/close on demand?
//        and does take ownership of read_from
// {{{ PDFTools::InStream
PDFTools::InStream::InStream(PDF &pdf,Dict *sdict,SubInput *read_from,const Ref *decryptref)
  : readfrom(read_from),
    decrypt(NULL),filter(NULL)
{
  try {
    dict._move_from(sdict);
    // assert(dict.find("Length")); // "contained" in SubInput

    const char *cryptname=NULL;

    //  /Length
    const Object *filterspec=dict.find("Filter");
    if (filterspec) {
      filter=new IFilter(pdf,*filterspec,dict.find("DecodeParms"));
      cryptname=filter->hasCrypt();
    }

    if (decryptref) {
      decrypt=pdf.getStmDecrypt(*decryptref,cryptname); 
    }
  } catch (...) {
    delete filter;
    delete decrypt;
    delete readfrom;
    throw;
  }
}

PDFTools::InStream::~InStream()
{
  delete filter;
  delete decrypt;
  delete readfrom;
}

void PDFTools::InStream::print(Output &out) const
{
  if (decrypt) {
    fprintf(stderr,"WARNING: Incorrect length in InStream::print\n");
  }
  dict.print(out);
  out.puts("\nstream\n");
  InputPtr in=const_cast<InStream &>(*this).open(true); // TODO: bad ...

  copy(out,in);
  out.puts("endstream"); //  out.puts("\nendstream");
}

const Dict &PDFTools::InStream::getDict() const
{
  return dict;
}

InputPtr PDFTools::InStream::open(bool raw)
{
  if (decrypt) {
    if ( (!raw)&&(filter) ) {
      return filter->open(decrypt->getInput(*readfrom),true); // will pos(0)
    } else {
      readfrom->pos(0);
      return InputPtr(decrypt->getInput(*readfrom),true);
    }
  } else if ( (!raw)&&(filter) ) {
    return filter->open(readfrom,false); // will pos(0)
  } else {
    readfrom->pos(0);
    return InputPtr(readfrom,false);
  }
}

void PDFTools::InStream::write(const char *filename) const
{
  InputPtr in=const_cast<InStream &>(*this).open(false); // TODO: bad ...
  if (!filename) {
    copy(stdfo,in);
  } else {
    FILEOutput fo(filename);
    copy(fo,in);
  }
}

SubInput *PDFTools::InStream::release()
{
  SubInput *ret=readfrom;
  readfrom=NULL;
  return ret;
}
// }}}

// TODO: when encryptMeta = false we have to *add*  /Filter[/Crypt] to supress decryption!
// {{{ PDFTools::OutStream
PDFTools::OutStream::OutStream(Input *read_from,bool take,Dict *sdict) : readfrom(read_from),ours(take),encrypt(NULL),filter(NULL)
{
  if (sdict) {
    dict._move_from(sdict);
  }
}

PDFTools::OutStream::~OutStream()
{
  delete filter;
  delete encrypt;
}

Ref PDFTools::OutStream::output(OutPDF &outpdf,bool raw)
{
  dict.erase("Length");
  if (!raw) {
    dict.erase("Filter");
    dict.erase("DecodeParms");
    if (filter) {
      const Object *fary=filter->getFilter();
      if (fary) {
        dict.add("Filter",fary);
        const Object *dpary=filter->getParams();
        if (dpary) {
          dict.add("DecodeParms",dpary);
        }
      }
    }
  }

  Ref ref=outpdf.newRef();
  if (  (!encrypt)&&( (raw)||(!filter) )  ) {
    int len=-1;
    if (MemInput *mip=dynamic_cast<MemInput *>(readfrom)) {
      len=mip->size();
    } else if (MemIOput *mip=dynamic_cast<MemIOput *>(readfrom)) {
      // if !filter && !encrypt
      len=mip->size();
    } // TODO  else if <InStream *> [bad: is InputPtr]  (but: DECODED [or raw] length)
    if (len!=-1) { // size already known (as long as write_base is byte-transparent)
      dict.add("Length",len);
      const int res=outpdf.outStream(dict,*readfrom,NULL,NULL,ref);
      assert(res==len); 
      return ref;
    }
  }

  Ref lref=outpdf.newRef();
  dict.add("Length",&lref);

  const int len=outpdf.outStream(dict,*readfrom,encrypt,(!raw)?filter:NULL,ref);

  outpdf.outObj(NumInteger(len),lref);
  return ref;
}

void PDFTools::OutStream::addDict(const char *key,const Object *obj,bool take)
{
  dict.add(key,obj,take);
}

OFilter &PDFTools::OutStream::ofilter()
{
  if (!filter) {
    filter=new OFilter;
  }
  return *filter;
}

void PDFTools::OutStream::addDict(const char *key,const char *name)
{
  dict.add(key,name,Name::STATIC);
}

void PDFTools::OutStream::setDict(const char *key,const char *nval)
{
  dict.erase(key);
  dict.add(key,nval,Name::STATIC);
}

void PDFTools::OutStream::setDict(const char *key,int ival)
{
  dict.erase(key);
  dict.add(key,ival);
}

void PDFTools::OutStream::setDict(const char *key,const Object &obj)
{
  dict.erase(key);
  dict.add(key,&obj,false);
}

void PDFTools::OutStream::setDict(const char *key,const std::vector<float> &nums)
{
  dict.erase(key);
  dict.add(key,Array::getNums(nums),true);
}

void PDFTools::OutStream::unsetDict(const char *key)
{
  dict.erase(key);
}
// }}}

#include "pdffilter_int.h"

// {{{ PDFTools::OutPDF
PDFTools::OutPDF::OutPDF(FILEOutput &write_base) : write_base(write_base),xref(true)
{
  version=13;
  write_header();
}

Ref PDFTools::OutPDF::newRef()
{
  return xref.newRef();
}

Ref PDFTools::OutPDF::outObj(const Object &obj)
{
  Ref ret=xref.newRef(write_base.sum());

  write_base.printf("%d %d obj\n",ret.ref,ret.gen);
  obj.print(write_base);
  write_base.puts("\nendobj\n");
  return ret;
}

void PDFTools::OutPDF::outObj(const Object &obj,const Ref &ref)
{
  xref.setRef(ref,write_base.sum());

  write_base.printf("%d %d obj\n",ref.ref,ref.gen);
  obj.print(write_base);
  write_base.puts("\nendobj\n");
}

int PDFTools::OutPDF::outStream(const Dict &dict,Input &readfrom,Encrypt *encrypt,OFilter *filter,const Ref &ref)
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

void PDFTools::OutPDF::finish(const Ref *pgref)
{
  if (pgref) {
    write_trailer(*pgref);
  } else {
    // PagesTree
    write_trailer(pages.output(*this));
  }
}

/* TODO ? howto generate id before 
std::string PDFTools::OutPDF::generate_id_first()
{
  ?
}

std::string PDFTools::OutPDF::generate_id_update()
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
*/

// remap/copy references from Array,Dict (modifies >obj)
void PDFTools::OutPDF::remap_array(PDF &inpdf,Array &aval,map<Ref,Ref> *donemap)
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

void PDFTools::OutPDF::remap_dict(PDF &inpdf,Dict &dval,map<Ref,Ref> *donemap)
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

/* copies the subtree below >startref
 * will return the Ref* to the startref-copy in the new tree
 *  or an Object *, if the reference can be made a direct object
 */
Object *PDFTools::OutPDF::copy_from(PDF &inpdf,const Ref &startref,map<Ref,Ref> *donemap)
{
  if (donemap) {
    map<Ref,Ref>::const_iterator it=donemap->find(startref);
  
    if (it!=donemap->end()) {
      return it->second.clone(); 
    }
  }

  auto_ptr<Object> obj(inpdf.fetch(startref));
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
        donemap->insert(make_pair(startref,ret));
      }
      return ret.clone();
    }
  } else if (Array *av=dynamic_cast<Array *>(obj.get())) {
    Ref ret=newRef();
    if (donemap) {
      donemap->insert(make_pair(startref,ret));
    }
    remap_array(inpdf,*av,donemap);

    outObj(*obj,ret);
    return ret.clone();
  } else if (Dict *dv=dynamic_cast<Dict *>(obj.get())) {
    Ref ret=newRef();
    if (donemap) {
      donemap->insert(make_pair(startref,ret));
    }
    remap_dict(inpdf,*dv,donemap);

    outObj(*obj,ret);
    return ret.clone();
  } else if (InStream *sv=dynamic_cast<InStream *>(obj.get())) {
    Dict &indict=const_cast<Dict &>(sv->getDict()); // BEWARE: we effectively neuter the InStream

    // FIXME: no newRef here ...  -- infinite remap loop is possible
    const bool rawcopy=false; //true;

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
      donemap->insert(make_pair(startref,ret));
    }
    return ret.clone();
  } else { // unknown type? - just remap
    printf("INFO: unexpected type: %s\n",typeid(*obj).name());
    Ref ret=outObj(*obj);
    if (donemap) {
      donemap->insert(make_pair(startref,ret));
    }
    return ret.clone();
  }
}

void PDFTools::OutPDF::write_header() const
{
  write_base.printf("%%PDF-%d.%d\n",version/10,version%10);
  write_base.puts("%\xe2\xe3\xc2\xd3\n"); // some binary stuff
}

void PDFTools::OutPDF::write_trailer(const Ref &pgref)
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
  xref.print(write_base,false);

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

