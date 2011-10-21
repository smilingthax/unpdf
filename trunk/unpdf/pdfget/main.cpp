#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "pdfbase.h"
#include "pdfparse.h"
#include "exception.h"
#include "cmdline.h"
#include <stdarg.h>
#include <stdio.h>

using namespace std;
using namespace PDFTools;

extern FILEOutput stdfo;

class PG_Cmdline : public Cmdline { // {{{
public:
  PG_Cmdline() : Cmdline(true,false,"[options] [input file] [[obj-ref]]\n") { // opt_opts, but we do --help ourselfes
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
       // "default": uncompressed content[only readable stuff is printed to stdout], and only dict for streams (? dict+raw);
    add(NULL,"raw",out_raw,"use raw format"); // no dict
    add(NULL,"image",out_image,"use corresponding image format");
    add(NULL,"hex",out_hex,"output as hex dump");
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
    if (outputfile=="-") {
      outputfile.clear(); // use stdout
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
    if (count_of("--raw","--image","--hex",NULL)>1) {
      printf("ERROR: Only one of --raw, --image and --hex may be given\n");
      ret=false;
    } else if (out_raw) {
      format=FORMAT_RAW;
    } else if (out_image) {
      format=FORMAT_IMAGE;
    } else if (out_hex) {
      format=FORMAT_HEX;
/* TODO;  Note that RAW is alreay decrypted!
    } else if (out_compressed) {
      format=FORMAT_COMPRESSED; */
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

  enum { FORMAT_DEFAULT, FORMAT_RAW, FORMAT_IMAGE, FORMAT_COMPRESSED, FORMAT_HEX } format;

  string inputfile;
  string outputfile;
private:
  bool mode_pages,mode_info,mode_root,mode_trailer;
  string mode_obj;

  bool out_raw,out_image,out_hex;

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

static bool printable_stream(PDF &pdf,InStream &stm) { // {{{ - if stream content should be printed by default
  try {
    const Dict &dict=stm.getDict();
    string stype=dict.getString(pdf,"Type");
    if (stype=="XObject") {
      string subtype=dict.getString(pdf,"Type");
      if (subtype=="Image") {
        return false;
      }
    }
  } catch (...) {
  }
  InputPtr in=stm.open(false);
  if (in.empty()) {
    return true;
  }
  char buf[256];
  int len=in.read(buf,256);
  for (int iA=0;iA<len;iA++) {
    if ( (!isprint(buf[iA]))&&(!isspace(buf[iA])) ) {
      return false;
    }
  }
  return true;
}
// }}}

static void hexdump(Output &out,Input &in) // {{{
{
  char buf[16];
  int iA,iB,pos=0;

  while (1) {
    iA=in.read(buf,16);
    if (iA<=0) {
      break;
    }

    out.printf("%08x  ",pos);
    for (iB=0;iB<iA;iB++) {
      out.printf("%02x ",(unsigned char)buf[iB]);
      if (iB==7) {
        out.puts(" ");
      }
    }
    for (;iB<16;iB++) {
      out.puts("   ");
      if (iB==7) {
        out.puts(" "); 
      }
    }
    out.puts(" ");
    for (iB=0;iB<iA;iB++) {
      if (isprint(buf[iB])) {
        out.put(buf[iB]);
      } else {
        out.put('.');
      }
    }
    out.put('\n');

    pos+=iA;
  }

  // return pos;
}
// }}}

// TODO: format:  raw(dict+data)/compressed binary  vs. PNG/JPEG/G4(/ZLIB)  vs.  PPM/PBM/TEXT(uncompressed)
// TODO: common crypt
// TODO: -o [file] ->fo /replace dump. maybe replace auto_ptr<Object> by ObjectPtr

/* FIXME
The IMAGE-format needs more thinking:
 Use-case 1: output uncompressed image data (pbm/ppm/...)
   [--image]
 Use-case 2: output compressed image data in a "comparable" format, i.e. lossy/lossless (esp. JPG,PNG)
   this probably especially means no recompression for JPG!
   [--jpg]
 Use-case 3: output compressed image data in "original" format, i.e. JPG, generate wrapper for Flate->PNG, G4(->TIFF?), LZW->TIFF
   might not always be possible! e.g. LZW with png predictor, etc.
   [--imgraw]

See also: class Image; in rletest/
*/

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

    FILEOutput fo(!cmdl.outputfile.empty()?cmdl.outputfile.c_str():NULL,stdout);
    // now output robj
    if (InStream *stmval=dynamic_cast<InStream *>(robj.get())) {
      stmval->getDict().print(stdfo);
      stdfo.put('\n');
      if (cmdl.format==PG_Cmdline::FORMAT_RAW) {
        InputPtr in=stmval->open(true); // TODO: false  (???)
        copy(fo,in);
        if (cmdl.outputfile.empty()) {
          stdfo.put('\n');
        }
      } else if (cmdl.format==PG_Cmdline::FORMAT_IMAGE) {
        printf("TODO: NOT IMPLEMENTED\n");
      } else if (cmdl.format==PG_Cmdline::FORMAT_COMPRESSED) {
        // TODO: FORMAT_COMPRESSED   ... decrypted but nothing more...
        printf("TODO: NOT IMPLEMENTED\n");
      } else if (cmdl.format==PG_Cmdline::FORMAT_HEX) {
        InputPtr in=stmval->open(false);
        hexdump(fo,in);
      } else { // FORMAT_DEFAULT
        if ( (!cmdl.outputfile.empty())||
             (printable_stream(*pdf,*stmval)) ) {
          InputPtr in=stmval->open(false);
          copy(fo,in);
        }
      }
    } else {
      if (!robj.get()) {
        stdfo.printf("NULL");
      } else {
        robj->print(fo);
      }
      stdfo.put('\n');
    }
    fo.flush();
  } catch (exception &ex) {
    printf("Ex: %s\n",ex.what());
  }

  return 0;
}
