#include "gfxfiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int read_pbm(const char *filename,unsigned char **buf,int *width,int *height)
{
  FILE *f=stdin;
  int iA,iB,plain;
  char tmp[256],c;
  unsigned char iC,*out;

  assert( (buf)&&(width)&&(height) );
  if (filename) {
    if ((f=fopen(filename,"r"))==NULL) {
      return -1;
    }
  }

  fread(tmp,2,1,f);
  if (tmp[0]!='P') {
    return -2;
  }
  if (tmp[1]=='1') { // P1
    plain=1;
  } else if (tmp[1]=='4') { // P4
    plain=0;
  } else {
    return -2;
  }

  // read header
  // skip to number
  do {
    c=getc(f);
    if (c=='#') {
      while (c!='\n') {
        c=getc(f);
      }
    }
  } while ( (c==' ')||(c=='\r')||(c=='\n')||(c=='\t') );
  // read width
  iA=0;
  while ( (c>='0')&&(c<='9') ) {
    iA=(iA*10)+(c-'0');
    c=getc(f);
  }
  if (!iA) {
    return -2;
  }
  // skip ws
  while ( (c==' ')||(c=='\r')||(c=='\n')||(c=='\t') ) {
    c=getc(f);
  }
  // read height
  iB=0;
  while ( (c>='0')&&(c<='9') ) {
    iB=(iB*10)+(c-'0');
    c=getc(f);
  }
  if (!iB) {
    return -2;
  }
  if ( (c!=' ')&&(c!='\r')&&(c!='\n')&&(c!='\t') ) {
    return -2;
  }

  // allocate buffer, if necessary
  if (*buf) {
    if (iA*iB>(*width)*(*height)) {
      free(*buf);
      *buf=malloc((iA+7)/8*iB);
    }
  } else {
    *buf=malloc((iA+7)/8*iB);
  }
  if (!*buf) { // malloc failed
    return -3;
  }
  *width=iA;
  *height=iB;

  if (plain) {
    out=*buf;
    for (;iB>0;iB--) {
      for (iA=*width;iA>0;) {
        *out=0;
        for (iC=0x80;(iC>0)&&(iA>0);iC>>=1,iA--) {
          do {
            c=fgetc(f);
          } while ( (c==' ')||(c=='\r')||(c=='\n')||(c=='\t') );
          if (c=='1') {
            *out|=iC;
          } else if (c!='0') {
            return -2;
          }
        }
        out++;
      }
    }
  } else {
    const int bwidth=(*width+7)/8;
    fread(*buf,bwidth,*height,f);
  }

  if (filename) {
    fclose(f);
  }
  return 0; // TODO: check returncodes
}

void writebits(FILE *f,unsigned char c,unsigned char endbit)
{
  unsigned char iA;
  for (iA=0x80;iA>endbit;iA>>=1) {
    if (c&iA) {
      putc('1',f);
    } else {
      putc('0',f);
    }
  }
}

int write_pbm(const char *filename,unsigned char *buf,int width,int height,int plain)
{
  FILE *f=stdout;
  int iA,iB;
  const int bwidth=(width+7)/8,bmod=((width-1)&7)+1;
  
  if (filename) {
    if ((f=fopen(filename,"w"))==NULL) {
      return -1;
    }
  }
  if (plain) { // P1
    fprintf(f,"P1 %d %d\n",width,height);
    for (iA=0;iA<height;iA++) {
      for (iB=1;iB<bwidth;iB++) {
        writebits(f,*buf++,0);
      }
      // incomplete byte
      writebits(f,*buf++,0x80>>bmod);
      putc('\n',f);
    }
  } else { // P4
    fprintf(f,"P4 %d %d\n",width,height);
    fwrite(buf,bwidth,height,f);
  }
  if (filename) {
    fclose(f);
  }
  return 0; // TODO: check returncodes
}

/*
 * TIFF HINTS:
Header:
  short byte_order_indication; 0x4949 or 0x4d4d; (01001001 or 01001101); Little-endian or Big-endian
  short version=42;
  ulong ptr_to_ifd; // SEEK_POS, must !=0

IFD: have to start on word boundaries! (=^= page / image). Baseline needs only to read first
  short no_of_tags_in_ifd; // must >=1
  TAGDATA tag_data[no_of_tags_in_ifd];
  ulong (@2+no_of_tags_in_ifd*12) ptr_to_ifd; // or 0
  
TAGDATA: 12 byte
  short tag_id;
  short datatype;  // 1=ubyte, 2=7bit ASCII,NUL-terminated (may multiple, using num_of_values(bytes)), 3=ushort, 4=ulong, 5=ulong + ulong, a/b
                   // maybe: 6=sbyte, 7=undef char, 8=sshort, 9=slong, 10=slong+slong, 11=float, 12=double
  ulong num_of_values;
  ulong ptr_to_values_data; // or special case - datatype*num_of_values <=4 byte - the valuedata itself

need much support! e.g. strips, maybe tiles, ColorInterpretation, maybe uncompressed mode,...
 */
