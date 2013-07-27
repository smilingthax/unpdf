#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "libnpdf/io/ptr.h"
#include "libnpdf/io/file.h"
#include "libnpdf/stream/pdfcomp.h" // FIXME
#include "libnpdf/pages/page.h"
#include "libnpdf/pdf/pdf.h"
#include "libnpdf/pdf/outpdf.h"
#include "libnpdf/util/util.h"
#include "exception.h"
#include "cmdline.h"
#include <stdarg.h>
#include <stdio.h>

#include "image.h"
#include "libnpdf/color/pdfcols.h"
#include "libnpdf/filter/pdffilter.h"

using namespace PDFTools;

namespace PDFTools {
  extern FILEOutput stdfo;
}

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
    add("q","none",out_none,"do not display stream");
    add(NULL,"raw",out_raw,"use raw format (decrypted, uncompressed)"); // (dict always to stdfo, even with -o)
    add(NULL,"image",out_image,"use corresponding image format");
    add(NULL,"hex",out_hex,"output as hex dump");
    add(NULL,"compressed",format_compressed,"i.e. decrypted; to be combined with --raw, --image or --hex");
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
        fprintf(stderr,"ERROR: excessive arguments\n");
        ret=false;
      }
    } else {
      ret=false;
      fprintf(stderr,"ERROR: A input-file must be given\n");
    }

    if (outputfile=="-") {
      outputfile.clear(); // use stdout
    }

    // mode
    if (count_of("--pages","--info","--root","--trailer","--page","--obj",NULL)>1) {
      fprintf(stderr,"ERROR: Only one of --pages, --info, --root, --trailer, --page and --obj may be given\n");
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
        fprintf(stderr,"ERROR: page number must be at least 1");
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
        fprintf(stderr,"ERROR: unrecognized object reference: \"%s\"\n",mode_obj.c_str());
        // parse error
        ret=false;
      }
    } else {
      mode=MODE_PAGES;
    }

    // format
    if (count_of("--none","--raw","--image","--hex",NULL)>1) {
      fprintf(stderr,"ERROR: Only one of --none, --raw, --image and --hex may be given\n");
      ret=false;
    } else if (out_raw) {
      format=FORMAT_RAW;
    } else if (out_image) {
      format=FORMAT_IMAGE;
    } else if (out_hex) {
      format=FORMAT_HEX;
    } else if (out_none) {
      format=FORMAT_NONE;
    } else {
      format=FORMAT_DEFAULT;
    }
    if ( (format_compressed)&&(format==FORMAT_DEFAULT) ) {
//      fprintf(stderr,"ERROR: --compressed must be combined with --raw, --image or --hex\n");
//      ret=false;
      format=FORMAT_RAW;
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

  enum { FORMAT_NONE, FORMAT_DEFAULT, FORMAT_RAW, FORMAT_IMAGE, FORMAT_HEX } format;
  bool format_compressed;

  std::string inputfile;
  std::string outputfile;
private:
  bool mode_pages,mode_info,mode_root,mode_trailer;
  std::string mode_obj;

  bool out_none,out_raw,out_image,out_hex;

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
    std::string stype=dict.getString(pdf,"Type");
    if (stype=="XObject") {
      std::string subtype=dict.getString(pdf,"Type");
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
   [--image --(re)compressed  or  --raw --compressed]
 Use-case 3: output compressed image data in "original" format, i.e. JPG, generate wrapper for Flate->PNG, G4(->TIFF?), LZW->TIFF
   BUT: might not always be possible! e.g. LZW with png predictor, etc.
   [--image --compressed  or  --raw --compressed]

See also: class Image; in rletest/
*/

ColorSpace *getColorspaceForImage(PDF &pdf,const Dict &dict,bool required=true) // {{{
{
  ObjectPtr cobj=dict.get(pdf,"ColorSpace");
  if (cobj.empty()) {
    if (required) {
      throw UsrError("Required key /ColorSpace not found");
    }
    return NULL;
  }
  std::auto_ptr<ColorSpace> ret(ColorSpace::parse(pdf,*cobj));
  if (dynamic_cast<PatternColorSpace *>(ret.get())) {
    throw UsrError("/Pattern colorspace not allowed for images");
  }
  return ret.release();
}
// }}}

bool getMaskDecode(const std::vector<float> &decode) // {{{
{
  if ( (decode[0]==0.0)&&(decode[1]==1.0) ) {
    return false;
  } else if ( (decode[0]==1.0)&&(decode[1]==0.0) ) {
    return true;
  }
  throw UsrError("Bad /Decode array [%f, %f] for /ImageMask");
}
// }}}

#define NOT_IMAGE -1
#define UNSUPPORTED -2

#include "libnpdf/filter/pdffilter_int.h"  // TODO TODO

int output_image(Output &fo,PDF &pdf,InStream &stmval,bool compressed) // {{{
{
  const Dict &dict=stmval.getDict();

  ObjectPtr tobj=dict.get(pdf,"Type");
  if (!tobj.empty()) {
    Name *tnval=dynamic_cast<Name *>(tobj.get());
    if ( (!tnval)||(strcmp(tnval->value(),"XObject")!=0) ) {
      return NOT_IMAGE;
    }
  }

  ObjectPtr sobj=dict.get(pdf,"Subtype");
  Name *snval=dynamic_cast<Name *>(sobj.get());
  if ( (!snval)||(strcmp(snval->value(),"Image")!=0) ) {
    return NOT_IMAGE;
  }

  int width=dict.getInt(pdf,"Width"); // reqd
  int height=dict.getInt(pdf,"Height"); // reqd
  int bpc=-1;
  std::auto_ptr<ColorSpace> cs;

  bool is_mask=dict.getBool(pdf,"ImageMask",false);

  IFilter *filter=stmval.getFilter();
  const ColorSpace *jpx_cs=NULL;
  if (filter) {
    jpx_cs=filter->isJPX();
  }

  // BitsPerComponent, ColorSpace
  int colorComponents;
  if (is_mask) {
    bpc=dict.getInt(pdf,"BitsPerComponent",1);
    if (bpc!=1) {
      // TODO? ignore for (jpx_cs)
      throw UsrError("BitsPerComponent must be 1 for ImageMasks");
    }
    // Mask and ColorSpace should not be specified
    colorComponents=1;   // TODO? use (sentinel)  MaskColorSpace ?
  } else if (jpx_cs) {
    cs.reset(getColorspaceForImage(pdf,dict,false));
    if (!cs.get()) {
      // cs.reset(...jpx_cs...); // FIXME(?) ownership or clone
      colorComponents=jpx_cs->numComps();
    } else {
      // TODO? do not ignore Decode in this case?
      colorComponents=cs->numComps();
    }

    bpc=filter->hasBpc();
    assert(bpc>0);
  } else {
    cs.reset(getColorspaceForImage(pdf,dict,true));
    assert(cs.get()); // because we required it
    colorComponents=cs->numComps();

    bpc=dict.getInt(pdf,"BitsPerComponent");
    if (!validBPC(bpc)) {
      throw UsrError("Bad BitsPerComponent: %d",bpc);
    }

    if (filter) {
      const int filter_bpc=filter->hasBpc();
      if (bpc!=filter_bpc) {
        throw UsrError("Inconsistend BitsPerComponent: %d vs. %d",bpc,filter_bpc);
      }
    }
  }

// TODO?! extra flag to extract mask instead, esp. for Array-case
  ObjectPtr mobj=dict.get(pdf,"Mask");
  std::vector<float> maskvec;
  if (!mobj.empty()) {
    if (is_mask) {
      throw UsrError("/Mask and /ImageMask cannot both be present");
    }
    if (Array *maval=dynamic_cast<Array *>(mobj.get())) {
      // array of ranges
      maskvec=maval->getNums(pdf,2*colorComponents);
      //...
//      return UNSUPPORTED;
    } else if (/*InStream *msval=*/dynamic_cast<InStream *>(mobj.get())) {
      // ref ...  to b/w image with /ImageMask set
// TODO...
//      return UNSUPPORTED;
    } else {
      throw UsrError("Unexpected /Mask value");
    }
  }

// UNUSED warning
//  bool invert_mask=false;
  std::vector<float> decode;
  if ( (is_mask)||(!jpx_cs) ) {
    ArrayPtr dcval=dict.getArray(pdf,"Decode",false);
    if (!dcval.empty()) {
      decode=dcval->getNums(pdf,2*colorComponents);
      if (is_mask) { // TODO? better /  for every  colorComponents=1/bpc=1  image?
//        invert_mask=getMaskDecode(decode);
//        decode.clear();
      }
    }
  }

// TODO: enum  /  parseIntent
// intent=dict.getNames(pdf,"Intent","","AbsoluteColorimetric","RelativeColorimetric","Saturation","Perceptual",NULL);
// if (intent==0)  intent = FROM_GSTATE;  //default RelativeColorimetric

// bool interpolate=dict.getBool("Interpolate",false);

// Alternates

  int smask_from_jpx=0; // TODO?! enum
  if (jpx_cs) {
    smask_from_jpx=dict.getInt(pdf,"SMaskInData",0);
    if ( (smask_from_jpx<0)||(smask_from_jpx>2) ) {
      throw UsrError("Bad value %d for /SMaskInData",smask_from_jpx);
    }
  }

  if (dict.find("SMask")) {
    // soft mask
    if (smask_from_jpx!=0) {
      throw UsrError("Cannot handle /SMask with /SMaskInData!=0");
    }
    //...
  }

// Name, StructParent, ID, OPI, Metadata, OC

  // ---- output ----

  if (jpx_cs) {
    fprintf(stderr,"JPEG2000 not supported\n");
    return UNSUPPORTED;
  }

  if (!decode.empty()) {
    // TODO? check for standard-decode, check for b/w-invert?
    fprintf(stderr,"/Decode not supported\n");
    return UNSUPPORTED;
  }

  if ( (compressed)&&(filter)&&
       (filter->chain().size()>1) ) {

    if (dynamic_cast<JpegFilter::FInput *>(filter->chain().front())) {
      InputPtr in=stmval.open(false);
      copy(fo,*filter->chain()[1]);
      return 0;
    }

/* does not make sense without decoding parameters --> tiff
if (dynamic_cast<FaxFilter::FInput *>(filter->chain().front())) {
  // ... /K ?  /Columns ?    -> FaxFilter::FInput::convertToTiff(...)  ?  -- at least friend...
  InputPtr in=stmval.open(false);
  copy(fo,*filter->chain()[1]);
  return 0;
}
*/

    // TODO? png? g4->tiff?
    fprintf(stderr,"Can only output jpg for now\n");
    return UNSUPPORTED;
  } else {
#if 1 // TODO?
    if (/*ICCColorSpace *iccs=*/dynamic_cast<ICCColorSpace *>(cs.get())) {
      ; // at least something...
    } else if (CieColorSpace *ccs=dynamic_cast<CieColorSpace *>(cs.get())) {
      if (ccs->ctype()==CieColorSpace::Lab) {
        return UNSUPPORTED;
      }
      // ... at least CalRGB  CalGray
    // } else if (IndexedColorSpace *idcs=dynamic_cast<IndexedColorSpace *>(cs.get())) { ...
    } else if (!dynamic_cast<SimpleColorSpace *>(cs.get())) {
      fprintf(stderr,"Can't handle non-/Device* Colorspace");
      return UNSUPPORTED;
    }
#endif

    InputPtr in=stmval.open(compressed);
    InputPtr more(NULL,false);

    // write pbm / pgm / ppm header
    switch (colorComponents) {
    case 1:
      switch (bpc) {
      case 1:
        fo.printf("P4\n"
                  "%d %d\n",
                  width, height);
// TODO? padding correction needed?
//        more.reset(new DecodeFilter::FInput(in,colorComponents,bpc,&decode,true),true);
        break;
      case 8:
      case 16:
        fo.printf("P5\n"
                  "%d %d\n"
                  "%d\n",
                  width,height,(1<<bpc)-1);
        break;
      default:
        fprintf(stderr,"Can't output grayscale with %d bits per component\n",bpc);
        return UNSUPPORTED;
      }
      break;
    case 3:
      switch (bpc) {
      case 8:
      case 16:
        fo.printf("P6\n"
                  "%d %d\n"
                  "%d\n",
                  width,height,(1<<bpc)-1);
        break;
      default:
        fprintf(stderr,"Can't output rgb with %d bits per component\n",bpc);
        return UNSUPPORTED;
      }
      break;
    case 4:
      switch (bpc) {
      case 8:
        // TODO.
//        break;
      default:
        fprintf(stderr,"Can't output CMYK with %d bits per component\n",bpc);
        return UNSUPPORTED;
      }
    default:
      fprintf(stderr,"Can't output with %d color components\n",colorComponents);
      return UNSUPPORTED;
    }

// TODO ?! decode-array?
    if (!more.empty()) {
      copy(fo,more);
    } else {
      copy(fo,in);
    }
  }

  return 0;
}
// }}}

int main(int argc,char **argv)
{
  PG_Cmdline cmdl;

  try {
    if (!cmdl.parse(argc,argv)) {
      return 1;
    }

    FILEInput fi(cmdl.inputfile.c_str());

    std::auto_ptr<PDF> pdf=open_pdf(fi);
    std::auto_ptr<Object> robj;

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
        InputPtr in=stmval->open(cmdl.format_compressed);
        copy(fo,in);
        if (cmdl.outputfile.empty()) {
          stdfo.put('\n');
        }
      } else if (cmdl.format==PG_Cmdline::FORMAT_IMAGE) {
        int res=output_image(fo,*pdf,*stmval,cmdl.format_compressed);
        if (res==0) {
          if (cmdl.outputfile.empty()) {
            stdfo.put('\n');
          }
        } else if (res==UNSUPPORTED) {
          fprintf(stderr,"ERROR: --image does not support this image or its parameters\n");
        } else {
          fprintf(stderr,"ERROR: Object is not an image\n");
        }
      } else if (cmdl.format==PG_Cmdline::FORMAT_HEX) {
        InputPtr in=stmval->open(cmdl.format_compressed);
        hexdump(fo,in);
      } else if (cmdl.format==PG_Cmdline::FORMAT_DEFAULT) {
        if ( (!cmdl.outputfile.empty())||
             (printable_stream(*pdf,*stmval)) ) {
          InputPtr in=stmval->open(false);
          copy(fo,in);
        }
      } // else FORMAT_NONE
    } else {
      if (!robj.get()) {
        stdfo.printf("NULL");
      } else {
        robj->print(fo);
      }
      stdfo.put('\n');
    }
    fo.flush();
  } catch (std::exception &ex) {
    fprintf(stderr,"Ex: %s\n",ex.what());
  }

  return 0;
}
