/**
 * LZW Encoder and Decoder
 * (c) 2007 by Tobias Hoffmann
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "gfxfiles.h"
#include "lzwcode.h"

void usage(const char *name)
{
  printf("LZW En-/Decoder\n"
         "(c) 2007 by Tobias Hoffmann\n\n"

         "Usage: %s [-d] [-pbmW] [-h] [-earlyE] [infile] [outfile]\n\n"

         "      -d: Decode\n"
         "   else : Encode\n\n"

         " -earlyE: Enlarge the code length E entries early (default: 1)\n\n"

         "   -pbmW: Read/Write pbm-file; using image width W (for decoding)\n\n"

         "      -h: Show this help\n\n"
//         "      -x: Read/Write hexstrings\n\n"

         "If outfile or both infile and outfile are not given\n" 
         "standard output and maybe standard input are used.\n\n"
         ,name);
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

int wrfunc_mem(void *user,unsigned char *buf,int len)
{
  char **tmp=(char **)user;
  memcpy(*tmp,buf,len);
  *tmp+=len;
  return 0;
}

int rdfunc_mem(void *user,unsigned char *buf,int len)
{
  char **tmp=(char **)user;
  memcpy(buf,*tmp,len);
  *tmp+=len;
  return 0;
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
  printf("\n");
  for (iA=0;iA<len;iA++) {
    printf("x%02x",buf[iA]);
  }
  printf("\n");

  return 0;
}

int main(int argc,char **argv)
{
  LZWSTATE *lzw;
  int ret=0,width,height,early=-1,decode=0,pbm=0;
  char *files[2]={NULL,NULL};
  unsigned char *buf=NULL,*tmp;
  int iA,iB;
  FILE *f=NULL,*g=NULL; // avoid warning

  // parse commandline
  iB=0;
  for (iA=1;iA<argc;iA++) {
    if (strcmp(argv[iA],"-d")==0) {
      decode=1;
    } else if (strncmp(argv[iA],"-early",5)==0) {
      if (argv[iA][5]) {
        early=atoi(argv[iA]+5);
      } else {
        early=-1;
      }
    } else if (strncmp(argv[iA],"-pbm",4)==0) {
      if (argv[iA][4]) {
        pbm=atoi(argv[iA]+4);
      } else {
        pbm=-1;
      }
    } else if ( (strcmp(argv[iA],"-h")==0)||(strcmp(argv[iA],"--help")==0) ) {
      usage(argv[0]);
      return 0;
    } else if (iB<2) {
      files[iB++]=argv[iA];
    } else {
      usage(argv[0]);
      return 1;
    }
  }
  if ( (decode)&&(pbm==-1) ) {
    fprintf(stderr,"Error: When using -d and -pbm the image width must be specified\n");
    return 1;
  }

#define BUFSIZE 4096
  if (decode!=0) {
    if (files[0]) {
      if ((f=fopen(files[0],"r"))==NULL) {
        fprintf(stderr,"Error opening \"%s\" for reading: %m\n",files[0]);
        return 2;
      }
    } else {
      f=stdin;
    }
    if (pbm) {
      width=(pbm+7)/8;
      iA=100; // initial alloc
      buf=malloc(iA*width);
      if (!buf) {
        fprintf(stderr,"Malloc failed: %m\n");
        if (files[0]) {
          fclose(f);
        }
        return 2;
      }
    } else {
      buf=malloc(BUFSIZE);
      if (!buf) {
        fprintf(stderr,"Malloc failed: %m\n");
        if (files[0]) {
          fclose(f);
        }
        return 2;
      }
      if (files[1]) {
        if ((g=fopen(files[1],"w"))==NULL) {
          fprintf(stderr,"Error opening \"%s\" for writing: %m\n",files[1]);
          if (files[0]) {
            fclose(f);
          }
          return 3;
        }
      } else {
        g=stdout;
      }
    }
    lzw=init_lzw_read(early,rdfunc,f);
    if (!lzw) {
      fprintf(stderr,"Alloc error: %m\n");
      free(buf);
      if (files[0]) {
        fclose(f);
      }
      if ( (!pbm)&&(files[1]) ) {
        fclose(g);
      }
      return 2;
    }
    // decode
    if (pbm) {
      height=0;
      while (1) {
        if (height>=iA) {
          iA+=iA;
          tmp=realloc(buf,iA*width);
          if (!tmp) {
            fprintf(stderr,"Realloc error: %m\n");
            ret=-1;
            break;
          } else {
            buf=tmp;
          }
        }
        ret=decode_lzw(lzw,buf+height*width,width);
        if (ret>0) { // done
          if (ret!=1) {
            fprintf(stderr,"Incomplete last line\n");
          }
          break;
        } else if (ret<0) {
          fprintf(stderr,"Decoder error: %d\n",ret);
          break;
        }
        height++;
      }
    } else {
      while (1) {
        ret=decode_lzw(lzw,buf,BUFSIZE);
        if (ret>0) { // done
          int len=ret-1;
          ret=fwrite(buf,1,len,g);
          if (ret!=len) {
            fprintf(stderr,"Write error: %m\n");
          }
          break;
        } else if (ret<0) {
          fprintf(stderr,"Decoder error: %d\n",ret);
          break;
        }
        ret=fwrite(buf,1,BUFSIZE,g);
        if (ret!=BUFSIZE) {
          fprintf(stderr,"Write error: %m\n");
          break;
        }
      }
    }
    free_lzw(lzw);
    if (files[0]) {
      fclose(f);
    }
    if (pbm) {
      ret=write_pbm(files[1],buf,pbm,height,0);
      if (ret) {
        fprintf(stderr,"PBM writer error: %d\n",ret); 
        free(buf);
        return 2;
      }
    } else if (files[1]) {
      fclose(g);
    }
    free(buf);
  } else { // encode
    if (pbm!=0) {
      ret=read_pbm(files[0],&buf,&width,&height);
      if (ret) {
        fprintf(stderr,"PBM reader error: %d\n",ret); 
        return 2;
      }
    } else {
      buf=malloc(BUFSIZE);
      if (!buf) {
        fprintf(stderr,"Malloc failed: %m\n");
        return 2;
      }
      if (files[0]) {
        if ((f=fopen(files[0],"r"))==NULL) {
          fprintf(stderr,"Error opening \"%s\" for reading: %m\n",files[0]);
          free(buf);
          return 2;
        }
      } else {
        f=stdin;
      }
    }
    if (files[1]) {
      if ((g=fopen(files[1],"w"))==NULL) {
        fprintf(stderr,"Error opening \"%s\" for writing: %m\n",files[1]);
        free(buf);
        if ( (!pbm)&&(files[0]) ) {
          fclose(f);
        }
        return 3;
      }
    } else {
      g=stdout;
    }
//    lzw=init_lzw_write(1,wrfunc_mem,&tmp);
    lzw=init_lzw_write(early,wrfunc,g);
    if (!lzw) {
      fprintf(stderr,"Alloc error: %m\n");
      free(buf);
      if ( (!pbm)&&(files[0]) ) {
        fclose(f);
      }
      if (files[1]) {
        fclose(g);
      }
      return 2;
    }
    // encode
    if (pbm) {
      ret=encode_lzw(lzw,buf,(width+7)/8*height);
    } else {
      int len;
      while ((len=fread(buf,1,BUFSIZE,f))>0) {
        ret=encode_lzw(lzw,buf,len);
        if (ret) {
          break;
        }
      }
    }
    free(buf);
    if (!ret) {
      ret=encode_lzw(lzw,NULL,0);
    }
    free_lzw(lzw);
    if ( (!pbm)&&(files[0]) ) {
      fclose(f);
    }
    if (files[1]) {
      fclose(g);
    }
    if (ret) {
      fprintf(stderr,"Encoder error: %d\n",ret);
      return 2;
    }
  }
  
  return 0;
}
