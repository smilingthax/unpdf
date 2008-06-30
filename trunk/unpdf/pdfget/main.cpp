#include <assert.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "exception.h"
#include "cmdline.h"
#include <stdarg.h>

using namespace std;
using namespace PDFTools;

extern FILEOutput stdfo;

class PG_Cmdline : public Cmdline { // {{{
public:
  PG_Cmdline() : Cmdline(true,false,"[options] [input file] [[obj ref]]\n") { // opt_opts, but we do --help ourselfes
    add_usage("Modes:");
    // mode
    add(NULL,"pages",mode_pages,"Output pagetoc"); // current default
    add(NULL,"info",mode_info,"Output info-dict");
    add(NULL,"root",mode_root,"Output root-dict");
    add(NULL,"trailer",mode_trailer,"Output trailer-dict");
    add_param("p","page",mode_page,"Output page [number]");
    add_param(NULL,"obj",mode_obj,"Output object [ref]"); // default if 2 args
    add_usage();
    // format
       // default: value, and only dict for streams (? dict+raw);
    add(NULL,"raw",out_raw,"use raw format"); // no dict
    add(NULL,"image",out_image,"use corresponding image format");
    add_usage();
    // 
    add_param("o",NULL,outputfile,"Output to [file]");
    add("h","help",do_usage);
  }
  bool parse(int argc,char const *const *argv) {
    bool ret=Cmdline::parse(argc,argv);
    if (opt_opts.size()>=1) {
      inputfile=opt_opts[0];
      if (opt_opts.size()==2) {
        if (ret) {
          set("--obj",opt_opts[1].c_str());
        }
      } else if (opt_opts.size()>2) {
        printf("ERROR: excessive arguments\n");
        ret=false;
      }
    } else {
      ret=false;
      printf("ERROR: A input-file must be given\n");
    }
    // mode
    if (count_of("--pages","--info","--root","--trailer","--page","--obj",NULL)>1) {
      printf("ERROR: Only one of --pages, --info, --root, --trailer, --page and --obj may be given\n");
      ret=false;
    } else if (mode_pages) {
      mode=MODE_PAGES;
    } else if (mode_info) {
      mode=MODE_INFO;
    } else if (mode_root) {
      mode=MODE_ROOT;
    } else if (mode_trailer) {
      mode=MODE_TRAILER;
    } else if (mode_page!=INT_MIN) {
      mode=MODE_PAGE;
      if (mode_page<=0) {
        printf("ERROR: page number must be at least 1");
        ret=false;
      }
    } else if (!mode_obj.empty()) {
      mode=MODE_OBJ;
      // parse format: "no" "no version" "no ver R"
      char *end;
      mode_ref.ref=strtoul(mode_obj.c_str(),&end,10);
      end+=strspn(end," ");
      if (*end) {
        mode_ref.gen=strtoul(end,&end,10);
        end+=strspn(end," ");
      }
      if (*end=='R') {
        end++;
        end+=strspn(end," ");
      }
      // TODO? allow multiple objects
      if (*end) {
        printf("ERROR: unrecognized object reference: \"%s\"\n",mode_obj.c_str());
        // parse error
        ret=false;
      }
    } else {
      mode=MODE_PAGES;
    }
    // format
    if (count_of("--raw","--image",NULL)>1) {
      printf("ERROR: Only one of --raw and --image may be given\n");
      ret=false;
    } else if (out_raw) {
      format=FORMAT_RAW;
    } else if (out_image) {
      format=FORMAT_IMAGE;
    } else {
      format=FORMAT_DEFAULT;
    }

    if (!ret) {
      usage();
      return ret;
    }
    return true;
  }

  enum { MODE_PAGES,MODE_INFO,MODE_ROOT,MODE_TRAILER,MODE_PAGE,MODE_OBJ } mode;
  int mode_page;
  Ref mode_ref;

  enum { FORMAT_DEFAULT, FORMAT_RAW, FORMAT_IMAGE } format;

  string inputfile;
  string outputfile;
private:
  bool mode_pages,mode_info,mode_root,mode_trailer;
  string mode_obj;

  bool out_raw,out_image;

protected:
  bool count_of(const char *first,...) { // {{{
    va_list va;

    int count=0;
    va_start(va,first);
    const char *tmp=first;
    do {
      if (!is_default(tmp)) {
        count++;
      }
      tmp=va_arg(va,const char *);
    } while (tmp);
    va_end(va);

    return (count==1);
  }
  // }}}
};
// }}}

// TODO? -root (single dash)

// TODO: format:  raw(dict+data)/compressed binary  vs. PNG/JPEG/G4(/ZLIB)  vs.  PPM/PBM/TEXT(uncompressed)
         // some streams should be output (page contents) as clear.
         // images should not be output to console [/Type /XObject /SubType /Image ??]
// TODO: common crypt
// TODO: -o [file] ->fo /replace dump. maybe replace auto_ptr<Object> by ObjectPtr

int main(int argc,char **argv)
{
  PG_Cmdline cmdl;

  try {
    if (!cmdl.parse(argc,argv)) {
      return 1;
    }

    FILEInput fi(cmdl.inputfile.c_str());

    auto_ptr<PDF> pdf=open_pdf(fi);
    auto_ptr<Object> robj;

    // assert(pdf->pages[*].getReadRef()!=NULL); // property of input-PDFs
    if (cmdl.mode==PG_Cmdline::MODE_PAGES) {
      printf("%d pages\n",pdf->pages.size());
      for (int iA=0;iA<(int)pdf->pages.size();iA++) {
        pdf->pages[iA].getReadRef()->print(stdfo);
//        printf("%d ",pdf->pages[iA].getReadRef()->ref);
        if (iA%10==9) {
          stdfo.put('\n');
        } else if (iA%10==4) {
          stdfo.puts("  ");
        } else {
          stdfo.put(' ');
        }
      }
      if (pdf->pages.size()%10!=0) {
        stdfo.put('\n');
      }
      return 0;
    } else if (cmdl.mode==PG_Cmdline::MODE_INFO) {
      // TODO? c++ interface for info
      const Ref *inforf=dynamic_cast<const Ref *>(pdf->trdict.find("Info"));
      if (inforf) {
        stdfo.puts("Info: ");
        inforf->print(stdfo);
        stdfo.put('\n');
        robj.reset(pdf->fetch(*inforf));
      } 
      if (!robj.get()) {
        printf("No info found.\n");
        return 0;
      }
    } else if (cmdl.mode==PG_Cmdline::MODE_ROOT) {
      dump(&pdf->rootdict);
      return 0;
    } else if (cmdl.mode==PG_Cmdline::MODE_TRAILER) {
      dump(&pdf->trdict);
      return 0;
    } else if (cmdl.mode==PG_Cmdline::MODE_PAGE) {
      // assert(cmdl.mode_page>0); // from Cmdline checking
      if (cmdl.mode_page>(int)pdf->pages.size()) {
        printf("Page number %d does not exist (last is %d)\n",cmdl.mode_page,pdf->pages.size());
        return 2;
      }
      robj.reset(pdf->fetch(*pdf->pages[cmdl.mode_page-1].getReadRef()));
    } else if (cmdl.mode==PG_Cmdline::MODE_OBJ) {
      robj.reset(pdf->fetch(cmdl.mode_ref));
    }

    // now output robj
if (InStream *stmval=dynamic_cast<InStream *>(robj.get())) {
  // TODO: filter.open() -> reset
  stmval->getDict().print(stdfo);
  stmval->write(NULL);
} else
    dump(robj.get());

/*
if (argc==3) {
  robj.reset(pdf->fetch(Ref(atoi(argv[2]),0)));
} else 
  robj.reset(pdf->fetch(Ref(4,0)));
      assert(robj.get());

      InStream *stmval=dynamic_cast<InStream *>(robj.get());
   // TODO: filter.open() -> reset
if (stmval) {
  stmval->getDict().print(stdfo);
  stmval->write(NULL);
} else
  dump(robj.get());
*/
  
  } catch (exception &ex) {
    printf("Ex: %s\n",ex.what());
  }

  return 0;
}
