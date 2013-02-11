#include "pdfcomp.h"
#include <assert.h>
#include <memory>
#include "exception.h"

#include "../pdf/pdf.h"
#include "../io/sub.h"
#include "../io/mem.h"
#include "../io/crypt.h"
#include "../util/util.h"

#include "../pdf/outpdf.h" // FIXME
#include "../filter/pdffilter.h" // FIXME
#include <stdio.h>

using namespace std;
using namespace PDFTools;

#include "../io/file.h"
namespace PDFTools {
  extern FILEOutput stdfo;
}

// TODO: InStream(...,SubInput *read_from,...)  // where >read_from is a separate FILEInput ? open/close on demand?
//        and does take ownership of read_from
// {{{ PDFTools::InStream
PDFTools::InStream::InStream(PDF *pdf,Dict *sdict,SubInput *read_from,const Ref *decryptref)
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

    if ( (pdf)&&(decryptref) ) {
      decrypt=pdf->getStmDecrypt(*decryptref,cryptname);
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

