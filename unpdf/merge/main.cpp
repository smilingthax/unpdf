#include <assert.h>
#include <stdlib.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "exception.h"
#include "cmdline.h"

using namespace std;
using namespace PDFTools;

class PM_Cmdline : public Cmdline { // {{{
public:
  PM_Cmdline() : Cmdline(true,false,"[-o outfile] infile[s]\n") { // opt_opts, but we do --help ourselves
    add_usage();

    add_param("o",NULL,outputfile,"Output to [file]");
    add("h","help",do_usage);
  }
  bool parse(int argc,char const *const *argv) {
    bool ret=Cmdline::parse(argc,argv);
    if (opt_opts.size()<1) {
      printf("ERROR: A input-file must be given\n");
      ret=false;
    }
    if (!ret) {
      usage();
      return ret;
    }
    return true;
  }

  string outputfile;
  using Cmdline::opt_opts;
};
// }}}

int main(int argc, char **argv)
{
  PM_Cmdline cmdl;

  try {
    if (!cmdl.parse(argc,argv)) {
      return 1;
    }

    // we first try to open all input files
    const int ilen=cmdl.opt_opts.size();
    vector<Input *> infiles;  // FIXME
    infiles.reserve(ilen);

    for (int iA=0;iA<ilen;iA++) {
      infiles.push_back(new FILEInput(cmdl.opt_opts[iA].c_str()));
    }

    FILEOutput fo(!cmdl.outputfile.empty()?cmdl.outputfile.c_str():NULL,stdout);
    OutPDF outpdf(fo);

    for (int iA=0;iA<ilen;iA++) {
      auto_ptr<PDF> pdf=open_pdf(*infiles[iA]);

      const int pno=pdf->pages.size();
//      map<Ref,Ref> donemap;
      for (int iB=0;iB<pno;iB++) {
//        outpdf.pages.add(outpdf,*pdf,iA,&donemap);
        outpdf.pages.add(outpdf,*pdf,iB,NULL);
      }
    }
    outpdf.finish();
    for (int iA=0;iA<ilen;iA++) {
      delete infiles[iA];
    }
  } catch (exception &e) {
    fprintf(stderr,"Exception: %s\n",e.what());
    return 1;
  }

  return 0;
}
