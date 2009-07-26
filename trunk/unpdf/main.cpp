/*
 * pdf stuff
 * (c) 2006(texttool),2007,2008 Tobias Hoffmann
 */
#include <stdio.h>
#include <stdexcept>
#include <string>
#include <stdlib.h>
#include <errno.h>
#include "pdfio.h"
#include "pdfbase.h"
#include "pdfparse.h"

#include "exception.h"
#include <assert.h>

using namespace std;
using namespace PDFTools;

extern FILEOutput stdfo;

/* TODO
 - remove clone() from Security / Decrypt
 (- PagesTree to write Pages with Inherited data as 6-tree)
*/



#if 0
// TODO: clone() for objects

// Rect::parse ? Rect is an Array... ; maybe: Rect * array.to_rect, hmm...

... ParsingInput over SubInput for object...!
  ... check if inside object(?) -> Substream...

//
  ..
    Dict rootdict;
//    PagesTree pages;

  ... assert(trailer.root(assert(type=catalog)));
  ... trailer.root.pages.count, trailer.root.pages.kids[].resources.xobject.?(type=image).

}
#endif

unsigned char clampc(int x)
{
  if (x<0) {
    return 0;
  } else if (x>255) {
    return 255;
  }
  return x;
}

int main(int argc,char **argv)
{
  const char *fn="ld2tst.pdf";

  if (argc>=2) {
    fn=argv[1];
  }

  try {
//    rp.read(fn);
    FILEInput fi(fn);

    auto_ptr<PDF> pdf=open_pdf(fi);
    {
      /* TODO: interpret trailer:
        /Root
        /Encrypt
        /Info(opt) */
   
auto_ptr<Object> robj;
  /*     pages[0].Resources.XObject."front()".(This is our image)
  */
      // TODO: autoresolve Ref's
      // TODO: make /Type first in Dict::print

// TODO: in Input / Output base provide pos()...return -1; remove all other impls.

      // TODO?? add reference to XRef into Ref. [good for writing PDFs!];  maybe even count them(?) [usage]
      // ? how to find out, which objects are used?  ? how to re-reference a pdf? 
      // TODO: /XRefStm in trailer for xref stream
const Ref *inforf=dynamic_cast<const Ref *>(pdf->trdict.find("Info"));
if (inforf) {
  printf("%d pages, Info: %d\n",pdf->pages.size(),inforf->ref);
} else {
  printf("%d pages, No info\n",pdf->pages.size());
}
for (int iA=0;iA<(int)pdf->pages.size();iA++) {
  printf("%d ",pdf->pages[iA].getReadRef()->ref);
}
printf("\n");
/*
{ FILEOutput fo("x.pdf");
  OutPDF op(fo);
  map<Ref,Ref> donemap;
  op.pages.add(op,*pdf,0,&donemap);
  op.finish();
}
*/
      
if (argc==3) {
  robj.reset(pdf->fetch(Ref(atoi(argv[2]),0)));
} else 
  robj.reset(pdf->fetch(Ref(4,0)));
      assert(robj.get());

      InStream *stmval=dynamic_cast<InStream *>(robj.get());
   // TODO: filter.open() -> reset
if (stmval) {
  if (0) {
    stmval->getDict().print(stdfo);
    FILEOutput fo("stream.out");
//    InputPtr ip=stmval->open(true);
    InputPtr ip=stmval->open(false);
//    copy(fo,ip);
    char buf[4];
    // CMYK -> RGB
    fo.printf("P6\n937 461\n255\n");
    for (int iA=0;iA<937*461;iA++) { 
      int res=ip.read(buf,4);
      assert(res==4);
      unsigned char cc=buf[0],cm=buf[1],cy=buf[2],ck=buf[3];
      buf[0]=clampc((255-ck)*(255-cc)/255);
      buf[1]=clampc((255-ck)*(255-cm)/255);
      buf[2]=clampc((255-ck)*(255-cy)/255);
      fo.write(buf,3);
    }
    fo.flush();
  } else {
    stmval->getDict().print(stdfo);
    stmval->write(NULL);
  }
} else
  dump(robj.get());
  

/*
      assert(stmval);
      stmval->write("stream.out");
*/

    }
#if 0
#if 1
      FILEInput fi("t");
      InflateInput ifi(fi);
      char buf[500];
      int len=ifi.read(buf,500);
      printf("\"%.*s\"\n",len,buf);
#else
      FILEOutput fo("t");
      DeflateOutput dfo(fo);

      dfo.printf("Test\n");
      dfo.flush();
#endif
#endif
  } catch (exception &ex) {
    printf("Ex: %s\n",ex.what());
  }

  return 0;
}
