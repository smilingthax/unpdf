#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "exception.h"
#include "cmdline.h"
#include <stdarg.h>

using namespace std;
using namespace PDFTools;

extern FILEOutput stdfo;

// TODO: share with pdfget
Ref parse_objref(const char *buf) // {{{
{
  // parse format: "no" "no version" "no ver R"
  char *end;
  Ref ret;
  ret.ref=strtoul(buf,&end,10);
  end+=strspn(end," ");
  if (*end) {
    ret.gen=strtoul(end,&end,10);
    end+=strspn(end," ");
  }
  if (*end=='R') {
    end++;
    end+=strspn(end," ");
  }
  // TODO? allow multiple objects
  if (*end) {
    printf("ERROR: unrecognized object reference: \"%s\"\n",buf);
    // parse error
    ret.ref=-1;
  }
  return ret;
}
// }}}

class PP_Cmdline : public Cmdline { // {{{
public:
  PP_Cmdline() : Cmdline(true,false,"[options] [pdf file] [obj ref]\n") { // opt_opts, but we do --help ourselfes
    add_usage("Modes:");
    // mode
    add(NULL,"add",mode_add,"Add into current pdf structure");
    add(NULL,"replace",mode_replace,"Replace in current pdf structure");
    add(NULL,"erase",mode_erase,"Erase from pdf structure (results in a probably incomplete pdf)");
    add_usage();
    add(NULL,"append",mode_append,"Add as pdf-update structure");
    add(NULL,"override",mode_override,"Replace through pdf-update structure");
    add(NULL,"hide",mode_hide,"Hide through pdf-update structure (see --erase)");
    add_usage();

    // options
    add_param("i",NULL,objectfile,"Read object from [file] instead of stdin");
    add(NULL,"force",force,"Allow adding an already existing resp. replacing a non-existing object");
    add_param(NULL,"delta",deltavec,"Output only the (to be appended) pdf-update into [file] (--append or --override)");
    // 
    add_param("o","copy",outputvec,"Do not modify input PDF, but make a modified copy (not together with --delta)"); // XOR delta... - or: delta + copy output two files(?)

    add("h","help",do_usage);
  }
  bool parse(int argc,char const *const *argv) {
    bool ret=Cmdline::parse(argc,argv);
    if (do_usage) {
      usage();
      return ret;
    }
    if (opt_opts.size()==2) {
      inputfile=opt_opts[0];

      obj=parse_objref(opt_opts[1].c_str());
      if (obj.ref==-1) {
        ret=false;
      }
    } else if (opt_opts.size()>2) {
      ret=false;
      printf("ERROR: excessive arguments\n");
    } else {
      ret=false;
      printf("ERROR: A pdf-file and an object reference must be given\n");
    }
    // mode
    if (count_of("--add","--replace","--erase","--append","--override","--hide",NULL)!=1) {
      printf("ERROR: Exactly one of --add, --replace, --erase, --append, --override and --hide must be given\n");
      ret=false;
    } else if (mode_add) {
      mode=MODE_ADD;
    } else if (mode_replace) {
      mode=MODE_REPLACE;
    } else if (mode_erase) {
      mode=MODE_ERASE;
    } else if (mode_append) {
      mode=MODE_APPEND;
    } else if (mode_override) {
      mode=MODE_OVERRIDE;
    } else if (mode_hide) {
      mode=MODE_HIDE;
    }
    if (force) {
      mode=(typeof(mode))(mode&FORCE_AND); // bad...
    }
    // honour --delta "" resp. --copy "" (in not modifying source)
    if (!deltavec.empty()) {
      if (deltavec[0].empty()) {
        printf("ERROR: empty argument for --delta not allowed\n");
        ret=false;
      } else if ((mode&DELTA_BIT)==0) {
        printf("ERROR: --delta is only allowed for --append and --override\n");
        ret=false;
      } else {
        deltafile=deltavec[0];
      }
    }
    if (!outputvec.empty()) {
      if (outputvec[0].empty()) {
        printf("ERROR: empty argument for --copy not allowed\n");
        ret=false;
      } else {
        outputfile=outputvec[0];
      }
    }
    if ( (!deltafile.empty())&&(!outputfile.empty()) ) {
      printf("ERROR: Either --delta or --copy may be given\n");
      ret=false;
    }

    if (!ret) {
      usage();
      return ret;
    }
    return true;
  }

  enum { ERASE_BIT=0x1, DELTA_BIT=0x2, MUST_EXIST_BIT=0x4, NOT_EXIST_BIT=0x8, FORCE_AND=0x3,
         MODE_ADD=NOT_EXIST_BIT, MODE_REPLACE=MUST_EXIST_BIT, MODE_ERASE=MUST_EXIST_BIT|ERASE_BIT,
         MODE_APPEND=NOT_EXIST_BIT|DELTA_BIT, MODE_OVERRIDE=MUST_EXIST_BIT|DELTA_BIT, MODE_HIDE=MUST_EXIST_BIT|ERASE_BIT|DELTA_BIT } mode;
  Ref obj;

  string inputfile,deltafile,objectfile;
  string outputfile;
private:
  bool mode_add,mode_replace,mode_erase,mode_append,mode_override,mode_hide,force;
  vector<string> deltavec,outputvec;

protected:
  bool count_of(const char *first,.../*,NULL*/) { // {{{
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

// TODO? pdfput --replace "39 0 R" pdffile < infile  ; resp. --replace -- files...

// TODO? remove extra * from usage: "[string] *"; e.g. "[file]"
// TODO? multi-object format: e.g. --process/--process_update [file] (replaces or adds as appropriate) [? use simplified pdf [xref]]
//     or:   ./pdfput --append file.pdf -- obj0file obj1file obj2file ... [having format: "5 0 obj\n...data..."]
// TODO? same for: ./pdfget --out obj%%% -- obj0ref obj1ref obj2ref ... [format! ?howto change dest. obj num?]

// NOTE: replacing with higher object generation than current... ["just add" (? does this work reliably?); document it]

// TODO: common crypt

int main(int argc,char **argv)
{
  PP_Cmdline cmdl;

  try {
    fprintf(stderr,"\nThis tool DOES NOT WORK yet.\n\n");
    if (!cmdl.parse(argc,argv)) {
      return 1;
    }

    FILEInput fi(cmdl.inputfile.c_str());

    auto_ptr<PDF> pdf=open_pdf(fi);
    auto_ptr<Object> robj;

    // assert(pdf->pages[*].getReadRef()!=NULL); // property of input-PDFs

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
