#include <assert.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "exception.h"

using namespace std;
using namespace PDFTools;

void do_it(PDF &pdf,int page,int pno) // {{{ stdout the images from >page to >page+>pno-1 from >pdf
{
  FILEOutput fo(stdout);
  OutPDF outpdf(fo);
  map<Ref,Ref> donemap;

  const Ref *oldpgref=dynamic_cast<const Ref *>(pdf.rootdict.find("Pages")); 
  Ref pgsref=outpdf.newRef();
  donemap.insert(make_pair(*oldpgref,pgsref));

  Array kids;
  for (int iA=0;iA<pno;iA++) {
    kids.add(outpdf.copy_from(pdf,*pdf.pages[page+iA].getReadRef(),&donemap)->clone(),true);
  }

  Dict pdict;
  pdict.add("Type","Pages",Name::STATIC);
  pdict.add("Count",(int)kids.size());
  pdict.add("Kids",&kids);

  outpdf.outObj(pdict,pgsref);

  outpdf.finish(&pgsref);
}
// }}}

int main(int argc, char **argv)
{
  if (argc!=4) {
    fprintf(stderr,"Usage: %s file page len\n",argv[0]);
    return 1;
  }
  try {
    FILEInput fi(argv[1]);
    auto_ptr<PDF> pdf=open_pdf(fi);

    int page=atoi(argv[2])-1,pno=atoi(argv[3]);
//    do_it(*pdf,atoi(argv[2])-1,atoi(argv[3]));

    FILEOutput fo(stdout);
    OutPDF outpdf(fo);
    map<Ref,Ref> donemap;
    for (int iA=0;iA<pno;iA++) {
      outpdf.pages.add(outpdf,*pdf,page+iA,&donemap);
    }
    outpdf.finish();
  } catch (exception &e) {
    fprintf(stderr,"Exception: %s\n",e.what());
    return 1;
  }

  return 0;
}
