/**
 * G3,G4 Encoder and Decoder
 * (c) 2006,2007 by Tobias Hoffmann
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "g4code.h"
#include "gfxfiles.h"

void usage(const char *name)
{
  printf("G3 and G4 En-/Decoder\n"
         "(c) 2006,2007 by Tobias Hoffmann\n\n"

         "Usage: %s [-g3|-g3K|-g4] [-decodeW] [-h -p -b] [infile] [outfile]\n\n"
         
         "     -g3: G3 1-dimensional code (default)\n"
         "    -g3K: G3 2-dimensional code, parameter K, e.g. -g32 for K=2\n"
         "     -g4: G4 (2-dim) code\n\n"

         "-decodeW: Decode to pbm-file, using image width W,\n"
         "          e.g. -decode1728 (default, if not given)\n"
         "  else  : Encode from pbm-file\n\n"

         "      -h: Show this help\n"
         "      -p: Write plain pbm\n"
         "      -b: Read/Write bitstrings\n\n"

         "Only K=2 (low resolution) and K=4 (high resolution) are standardized.\n"
         "If outfile or both infile and outfile are not given\n" 
         "standard output and maybe standard input are used.\n\n"
         ,name);
  // TODO: maybe accept -g3 -2d
}

int wrfunc(void *user,unsigned char *buf,int len)
{
  FILE *f=(FILE *)user;
 
  return (fwrite(buf,1,len,f)==len)?0:1;
}

int rdfunc(void *user,unsigned char *buf,int len)
{
  FILE *f=(FILE *)user;
 
  return (fread(buf,1,len,f)==len)?0:1;
}

int wrfunc_bits(void *user,unsigned char *buf,int len)
{
  FILE *f=(FILE *)user;
  int iA;
  unsigned char iB;
  
  for (iA=0;iA<len;iA++) {
    for (iB=0x80;iB>0;iB>>=1) {
      if (buf[iA]&iB) {
        putc('1',f);
      } else {
        putc('0',f);
      }
    }
  }
  return 0;
}

int rdfunc_bits(void *user,unsigned char *buf,int len)
{
  FILE *f=(FILE *)user;
  int iA,c;
  unsigned char iB;

  for (iA=0;iA<len;iA++) {
    buf[iA]=0;
    for (iB=0x80;iB>0;iB>>=1) {
      do {
        c=getc(f);
      } while ( (c=='\r')||(c=='\n') );
      if (c=='1') {
        buf[iA]|=iB;
      } else if (c==-1) { // EOF: pad zero
        break;
      } else if (c!='0') {
        return 1;
      }
    }
  }
  return 0;
}

int main(int argc,char **argv)
{
  G4STATE *gst;
  int ret=0,k=0,decode=-1,width,height,plain=0,bits=0;
  char *files[2]={NULL,NULL};
  unsigned char *buf=NULL,*tmp;
  int iA,iB;
  FILE *f;

  // parse commandline
  iB=0;
  for (iA=1;iA<argc;iA++) {
    if (strncmp(argv[iA],"-g3",3)==0) {
      if (argv[iA][3]) {
        k=atoi(argv[iA]+3);
      } else {
        k=0;
      }
    } else if (strcmp(argv[iA],"-g4")==0) {
      k=-1;
    } else if (strncmp(argv[iA],"-decode",7)==0) {
      if (argv[iA][7]) {
        decode=atoi(argv[iA]+7);
      } else {
        decode=0;
      }
    } else if ( (strcmp(argv[iA],"-h")==0)||(strcmp(argv[iA],"--help")==0) ) {
      usage(argv[0]);
      return 0;
    } else if (strcmp(argv[iA],"-p")==0) {
      plain=1;
    } else if (strcmp(argv[iA],"-b")==0) {
      bits=1;
    } else if (iB<2) {
      files[iB++]=argv[iA];
    } else {
      usage(argv[0]);
      return 1;
    }
  }

  if (decode>=0) { // decode
    const int bwidth=(decode+7)/8;
    width=decode;
    iA=100; // initial alloc
    buf=malloc(iA*bwidth);
    if (!buf) {
      fprintf(stderr,"Malloc failed: %m\n");
      return 2;
    }
    if (files[0]) {
      if ((f=fopen(files[0],"r"))==NULL) {
        fprintf(stderr,"Error opening \"%s\" for reading: %m\n",files[0]);
        free(buf);
        return 3;
      }
    } else {
      f=stdin;
    }
    gst=init_g4_read(k,decode,(bits)?rdfunc_bits:rdfunc,f);
    if (!gst) {
      fprintf(stderr,"Alloc error: %m\n");
      if (files[0]) {
        fclose(f);
      }
      free(buf);
      return 2;
    }
    height=0;
    while (1) {
      if (height>=iA) {
        iA+=iA;
        tmp=realloc(buf,iA*bwidth);
        if (!tmp) {
          fprintf(stderr,"Realloc error: %m\n");
          ret=-1;
        } else {
          buf=tmp;
        }
      }
      if (!ret) {
        ret=decode_g4(gst,buf+height*bwidth);
        if (ret==1) { // done 
          break;
        } else if (ret<0) {
          fprintf(stderr,"Decoder error: %d\n",ret);
        }
      }
      if (ret) { // error
        // Try to write partial result
        free_g4(gst);
        if (files[0]) {
          fclose(f);
        }
        ret=write_pbm(files[1],buf,width,height,plain);
        if (ret) {
          fprintf(stderr,"PBM writer error: %d\n",ret); 
        }
        free(buf);
        return 2;
      }
      height++;
    }
    free_g4(gst);
    if (files[0]) {
      fclose(f);
    }
    ret=write_pbm(files[1],buf,width,height,plain);
    free(buf);
    if (ret) {
      fprintf(stderr,"PBM writer error: %d\n",ret); 
      return 2;
    }
  } else { // encode
    ret=read_pbm(files[0],&buf,&width,&height);
    if (ret) {
      fprintf(stderr,"PBM reader error: %d\n",ret); 
      return 2;
    }
    if (files[1]) {
      if ((f=fopen(files[1],"w"))==NULL) {
        fprintf(stderr,"Error opening \"%s\" for writing: %m\n",files[1]);
        free(buf);
        return 3;
      }
    } else {
      f=stdout;
    }
    gst=init_g4_write(k,width,(bits)?wrfunc_bits:wrfunc,f);
    if (!gst) {
      fprintf(stderr,"Alloc error: %m\n");
      free(buf);
      if (files[1]) {
        fclose(f);
      }
      return 2;
    }
    // encode
    {
      const int bwidth=(width+7)/8;
      for (iA=0;iA<height;iA++) {
        ret=encode_g4(gst,buf+bwidth*iA);
        if (ret) {
          fprintf(stderr,"Encoder error: %d\n",ret);
          free(buf);
          free_g4(gst);
          if (files[1]) {
            fclose(f);
          }
          return 2;
        }
      }
    }
    free(buf);
    ret=encode_g4(gst,NULL);
    free_g4(gst);
    if (bits) {
      fprintf(f,"\n");
    }
    if (files[1]) {
      fclose(f);
    }
    if (ret) {
      fprintf(stderr,"Encoder error: %d\n",ret);
      return 2;
    }
  }
  return 0;
}
