#include "util.h"
#include <assert.h>
#include <stdio.h>
#include "../io/file.h"
#include "../objs/base.h"

//#include "../io/tokenize.h" // or parse?

//#include "../objs/base.h"

#include "../io/pdfio.h" // FIXME
//#include "../parse/pdfparse.h" // FIXME

namespace PDFTools {

void preprint(ParsingInput &in) // {{{
{
  char buf[10];
  int res=in.read(buf,10);
  printf("\"");
  for (int iA=0;iA<res;iA++) {
    if (buf[iA]<32) {
      printf("\\x%02x",(unsigned char)buf[iA]);
    } else {
      putchar(buf[iA]);
    }
  }
  printf("\"\n");
  in.unread(buf,res);
}
// }}}

void dump(const Object *obj) // {{{
{
  if (!obj) { 
    printf("NULL\n");
    return;
  }
  FILEOutput fo(stdout);
  obj->print(fo);
  fo.put('\n');
}
// }}}

FILEOutput stdfo(stdout);

/* TODO : FILEInput
// {{{ copy(Output &out,FILE *f,len)  - copy >len bytes from >f to >out
void copy(Output &out,FILE *f,int len)
{
  assert( (f)&&(len>=0) );
  char buf[4096];
  int iA;
 
  while (len>0) {
    if (len>4096) {
      iA=fread(buf,1,4096,f);
    } else {
      iA=fread(buf,1,len,f);
    }
    if (iA<=0) {
      assert(0);
      break;
    }
    out.write(buf,iA);
    len-=iA;
  }
}
// }}}
*/

int copy(Output &out,Input &in,int len) // {{{
{
  char buf[4096];
  int iA;

  if (len<0) {
    len=0;
    do {
      iA=in.read(buf,4096);
      out.write(buf,iA);
      len+=iA;
    } while (iA>0);
    return len;
  }
  int ret=0;
  while (len>0) {
    if (len>=4096) {
      iA=in.read(buf,4096);
    } else {
      iA=in.read(buf,len);
    }
    if (iA<=0) {
      break;
    }
    out.write(buf,iA);
    len-=iA;
    ret+=iA;
  }
  return ret;
}
// }}}

} // namespace PDFTools
