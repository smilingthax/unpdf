#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "g4code.h"
#include "tables.h"

// init functions
G4STATE *init_g4(int kval,int width,READFUNC rf,WRITEFUNC wf,void *user_read,void *user_write)
{
  G4STATE *ret;

  if (width<=0) { // use default
    width=1728;
  }

  ret=malloc(sizeof(G4STATE));
  if (!ret) {
    return NULL;
  }
  ret->read=rf;
  ret->write=wf;
  ret->user_read=user_read;
  ret->user_write=user_write;
  ret->width=width;
  ret->kval=kval;
  ret->lastline=malloc(sizeof(int)*(width+2));
  if (!ret->lastline) {
    free(ret);
    return NULL;
  }
  ret->curline=malloc(sizeof(int)*(width+2));
  if (!ret->curline) {
    free(ret->lastline);
    free(ret);
    return NULL;
  }
  restart_g4(ret);
  return ret;
}

G4STATE *init_g4_read(int kval,int width,READFUNC rf,void *user_read)
{
  assert(rf);
  if (!rf) {
    return 0;
  }
  return init_g4(kval,width,rf,NULL,user_read,NULL);
}

G4STATE *init_g4_write(int kval,int width,WRITEFUNC wf,void *user_write)
{
  assert(wf);
  if (!wf) {
    return 0;
  }
  return init_g4(kval,width,NULL,wf,NULL,user_write);
}

void restart_g4(G4STATE *state)
{
  assert(state);
  if (state) {
    memset(state->lastline,0,sizeof(int)*(state->width+2));
    state->lastline[0]=state->width;
    state->lines_done=0;
    state->bitpos=0;
    state->bitbuf=0;
  }
}

void free_g4(G4STATE *state)
{
  if (state) {
    free(state->lastline);
    free(state->curline);
    free(state);
  }
}

// helper functions
int writecode(G4STATE *state,ENCHUFF *table,int code)
{
  unsigned char buf[4];
  int iA=0;

  // TODO? make tables LSB-aligned(or ints)
  state->bitbuf|=(unsigned)(table[code].bits<<16)>>state->bitpos;
  state->bitpos+=table[code].len;
  while (state->bitpos>=8) {
    buf[iA++]=state->bitbuf>>24;
    state->bitbuf<<=8;
    state->bitpos-=8;
  }
  if (!iA) {
    return 0;
  }
  return (*state->write)(state->user_write,buf,iA);
}

int writeflush(G4STATE *state)
{
  unsigned char buf[4];
  int iA=0;

  while (state->bitpos>0) {
    buf[iA++]=state->bitbuf>>24;
    state->bitbuf<<=8;
    state->bitpos-=8;
  }
  state->bitpos=0;
  if (!iA) {
    return 0;
  }
  return (*state->write)(state->user_write,buf,iA);
}

int writehuff(G4STATE *state,int black,int num)
{
  ENCHUFF *colorhuff[]={whitehuff,blackhuff};
  int ret=0;
  
  while (num>=2560) {
    ret=writecode(state,colorhuff[black],63+2560/64);
    if (ret) {
      return ret;
    }
    num-=2560;
  }
  if (num>=64) {
    ret=writecode(state,colorhuff[black],63+num/64);
    if (ret) {
      return ret;
    }
    num%=64;
  }
  return writecode(state,colorhuff[black],num);
}

int next_bits(G4STATE *state,int bits)
{
  int ret,iA;
  unsigned char buf[4];

  if (state->bitpos<bits) { // ensure enough bits
    int num=(bits-state->bitpos+7)>>3;
    ret=(*state->read)(state->user_read,buf,num);
    if (ret) {
      return -MAX_OP;
    }
    for (iA=0;iA<num;iA++) {
      state->bitbuf|=buf[iA]<<(24-state->bitpos);
      state->bitpos+=8;
    }
  }
  return state->bitbuf>>(32-bits);
}

void eat_bits(G4STATE *state,int bits)
{
  state->bitpos-=bits;
  state->bitbuf<<=bits;
}

int readcode(G4STATE *state,unsigned short *table,int bits)
{
  int ip=0,data,len=0;

  data=next_bits(state,bits);
  if (data<0) {
    return data;
  }
  while (((ip=table[ip+data])&0xf000)==0) {
    eat_bits(state,bits);
    len+=bits;
    data=next_bits(state,bits);
    if (data<0) { // read error (-MAX_OP)
      return data;
    }
  }
  eat_bits(state,(ip>>12)-len);
  if ((ip&0xfff)>2560) { // OPCODE
    return ip|0xfffff000;
  } else {
    return ip&0xfff;
  }
}

int readhuff(G4STATE *state,int black)
{
  unsigned short *colortable[]={whitehufftable,blackhufftable};
  int ret,val=0;
 
  while (1) {
    ret=readcode(state,colortable[black],DECODE_COLORHUFF_BITS);
    if (ret<0) { // maybe: -1,-2,-3; read error: -MAX_OP
      return ret;
    } else if (ret<64) {
      return ret+val;
    } else if ( (ret<103)&&(val%2560) ) { // error with bigmakeup: expected no 2560 any more 
      return -1-MAX_OP; // wrong_code error
    }
    val+=ret;
  }
  return val;
}

void rle_encode(int *line,const unsigned char *inbuf,int width)
{
  unsigned int ip;
  int pos;

  if (*inbuf&0x80) {
    *line++=0;
    ip=rlecode[*inbuf^0xff];
  } else {
    ip=rlecode[*inbuf];
  }
  pos=ip&0xf;
  while (pos<width) {
    ip>>=4;
    if (ip==0) {
      if (*inbuf&1) {
        ++inbuf;
        if ((*inbuf&0x80)==0) {
          *line++=pos;
          ip=rlecode[*inbuf];
        } else {
          ip=rlecode[*inbuf^0xff];
        }
      } else {
        ++inbuf;
        if (*inbuf&0x80) {
          *line++=pos;
          ip=rlecode[*inbuf^0xff];
        } else {
          ip=rlecode[*inbuf];
        }
      }
    } else {
      *line++=pos;
    }
    pos+=ip&0xf;
  }
  *line++=width;
}

void rle_decode(const int *line,unsigned char *outbuf,int width)
{
  static const unsigned char rletab[8]={0x00,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe};
  int pos=0,black=0;

  memset(outbuf,0,(width+7)/8);
  for (;*line<=width;line++) {
    while (*line>pos) {
      const int bitpos=pos&7;
      if (*line-pos>=8-bitpos) {
        if (black) {
          *outbuf|=0xff>>bitpos;
        }
        outbuf++;
        pos+=8-bitpos;
      } else {
        if (black) {
          *outbuf|=rletab[*line-pos]>>bitpos;
        }
        pos=*line;
      }
    }
    black^=1;
  }
}

int encode_line_2d(G4STATE *state)
{
  int black=0,a0,*curpos,*lastpos,ret;

  a0=0;
  curpos=state->curline; // a1
  lastpos=state->lastline; // b1
  while (*curpos<=state->width) {
    int iA=*lastpos-*curpos;
    if ( (*lastpos<state->width)&&(lastpos[1]<*curpos) ) { // b2<a1
      if ((ret=writecode(state,opcode,-OP_P))) {
        return ret;
      }
      a0=lastpos[1];
    } else if ( (iA>=-3)&&(iA<=3) ) {
      if ((ret=writecode(state,opcode,iA-OP_V))) {
        return ret;
      }
      a0=*curpos;
      if (a0>=state->width) {
        break;
      }
      curpos++;
      black^=1;
      if ( (lastpos>state->lastline)&&(lastpos[-1]>a0) ) { // maybe previous is still interesting!
        lastpos--;
      } else if (*lastpos<state->width) {
        lastpos++;
      }
    } else {
      if ((ret=writecode(state,opcode,-OP_H))) {
        return ret;
      }
      if ((ret=writehuff(state,black,*curpos-a0))) {
        return ret;
      }
      a0=*curpos;
      if (a0<state->width) { // otherwise "generate a 0"
        curpos++;
      }
      if ((ret=writehuff(state,black^1,*curpos-a0))) {
        return ret;
      }
      a0=*curpos;
      if (a0>=state->width) {
        break;
      }
      curpos++;
    }
    while ( (*lastpos<state->width)&&(*lastpos<=a0) ) { // update lastpos
      lastpos++;
      if (*lastpos<state->width) {
        lastpos++;
      }
    }
  }
  return 0;
}

int encode_line_1d(G4STATE *state)
{
  int black=0,a0,*curpos,ret;

  a0=0; 
  curpos=state->curline; // a1
  do {
    if ((ret=writehuff(state,black,*curpos-a0))) {
      return ret;
    }
    black^=1;
    a0=*curpos;
    curpos++;
  } while (a0<state->width);
  return 0;
}

int decode_line_2d(G4STATE *state)
{
  int black=0,a0,*curpos,*lastpos,ret;

  a0=0;
  curpos=state->curline; // a1
  lastpos=state->lastline; // b1
  do {
    ret=readcode(state,opcodetable,DECODE_OPCODE_BITS);
    if (ret==-1) {
      return -ERR_UNKNOWN_CODE;
    } else if (ret==-MAX_OP) {
      return -ERR_READ;
    } else if (ret==OP_P) {
      a0=lastpos[1];
    } else if (ret==OP_H) {
      // read more
      ret=readhuff(state,black);
      if (ret==-1) {
        return -ERR_UNKNOWN_CODE;
      } else if (ret==-MAX_OP) {
        return -ERR_READ;
      } else if (ret<0) { // FILL,EOL,wrong_bigmakeup
        return -ERR_WRONG_CODE;
// TODO: check ret==0
      }
      a0+=ret;
      *curpos++=a0;
      ret=readhuff(state,black^1);
      if (ret==-1) {
        return -ERR_UNKNOWN_CODE;
      } else if (ret==-MAX_OP) {
        return -ERR_READ;
      } else if (ret<0) { // FILL,EOL,wrong_bigmakeup
        return -ERR_WRONG_CODE;
// TODO: check ret==0
      }
      a0+=ret;
      *curpos++=a0;
    } else if ( (ret>=OP_VL3)&&(ret<=OP_VR3) ) { // OP_V..
      a0=*lastpos+(ret-OP_V);
      assert(a0<=state->width);
      *curpos++=a0;
      black^=1;
      if ( (lastpos>state->lastline)&&(lastpos[-1]>a0) ) { // maybe previous is still interesting!
        lastpos--;
      } else if (*lastpos<state->width) {
        lastpos++;
      }
    } else if (ret==EOL) {
      if (state->kval==-1) { // G4
        if (next_bits(state,12)!=0x001) {
          return -ERR_WRONG_CODE;
        }
        eat_bits(state,12);
        return 1; // done
      } else { // G3 2d, TODO? hmm eol in 2d code...
        assert(0);
        return -ERR_WRONG_CODE;
      }
    } else { // OP_EXT
      assert(0); 
      return -ERR_UNKNOWN_CODE;
    }
    while ( (*lastpos<state->width)&&(*lastpos<=a0) ) { // update lastpos
      lastpos++;
      if (*lastpos<state->width) {
        lastpos++;
      }
    }
  } while (a0<state->width);
//  printf("%d\n",a0);
  assert(a0==state->width);
  *curpos++=state->width+1;
  return 0;
}

int decode_line_1d(G4STATE *state)
{
  int black=0,a0,*curpos,ret;

  a0=0; 
  curpos=state->curline; // a1
  do {
    ret=readhuff(state,black);
//  printf("%da%x\n",black,ret);
    if (ret==-1) {
      return -ERR_UNKNOWN_CODE;
    } else if (ret==EOL) {
      if (curpos==state->curline) { // EOL EOL ... 
        int iA;
        for (iA=1;iA<0;iA++) { // TODO: how many EOLs are checked (? last line's EOL counted?)
          if (next_bits(state,12)!=0x001) {
            return -ERR_WRONG_CODE;
          }
          eat_bits(state,12);
        }
        return 1;
      } 
      // a0<state->width! TODO? not enough, maybe graceful!
//      *curpos++=state->width;
      assert(0);
      return -ERR_WRONG_CODE;
    } else if (ret==FILL) {
      // TODO? handle FILL
      return -ERR_UNKNOWN_CODE;
    } else if (ret==-MAX_OP) {
      return -ERR_READ;
    } else if (ret<0) { // namely: BIGMAKEUP-sequence wrong
      return -ERR_WRONG_CODE;
    } else if ( (ret==0)&&(curpos!=state->curline) ) { // namely: futile rle encoding. might overflow buffers
      return -ERR_WRONG_CODE;
    }
    a0+=ret;
    *curpos++=a0;
    black^=1;
  } while (a0<state->width);
  assert(a0==state->width);
  *curpos++=state->width+1;
  return 0;
}

void swap_lines(G4STATE *state)
{
  // swap lastline, curline
  int *tmp=state->curline;
  state->curline=state->lastline;
  state->lastline=tmp;

  state->lines_done++;
}

// main procedures
int encode_g4(G4STATE *state,const unsigned char *inbuf)
{
  int ret=0,iA;

  assert(state);
  if ( (!state)||(!state->write) ) {
    return -ERR_INVALID_ARGUMENT;
  }
  if (!inbuf) { // flush
    if (state->kval==-1) { // G4: EOL EOL
      if ((ret=writecode(state,opcode,-EOL))) {
        return -ERR_WRITE;
      }
      if ((ret=writecode(state,opcode,-EOL))) {
        return -ERR_WRITE;
      }
    } else if (state->kval==0) { // G3 1d
      for (iA=0;iA<7;iA++) { // TODO? customize how many EOL's are to be written
        if ((ret=writecode(state,opcode,-EOL))) {
          return -ERR_WRITE;
        }
      }
    } else { // G3 2d
      for (iA=0;iA<7;iA++) {
        if ((ret=writecode(state,opcode,-EOL1))) {
          return -ERR_WRITE;
        }
      }
    }
    // "pad to byte boundary" and flush
    writeflush(state);
    return 0;
  }
  rle_encode(state->curline,inbuf,state->width);

  if (state->kval==-1) { // G4
    ret=encode_line_2d(state);
  } else if (state->kval==0) { // G3 1d
    if ((ret=writecode(state,opcode,-EOL))) {
      return -ERR_WRITE;
    }
    ret=encode_line_1d(state);
  } else {
    if (state->lines_done%state->kval!=0) {
      if ((ret=writecode(state,opcode,-EOL0))) {
        return -ERR_WRITE;
      }
      ret=encode_line_2d(state);
    } else {
      if ((ret=writecode(state,opcode,-EOL1))) {
        return -ERR_WRITE;
      }
      ret=encode_line_1d(state);
    }
  }

  swap_lines(state);
  return ret;
}

int decode_g4(G4STATE *state,unsigned char *outbuf)
{
  int ret=0;

  assert(state);
  assert(outbuf);
  if ( (!state)||(!state->read)||(!outbuf) ) {
    return -ERR_INVALID_ARGUMENT;
  }
  if (state->kval>=0) {
    // read EOL on G3
    if (next_bits(state,12)!=0x001) {
      return -ERR_WRONG_CODE;
    }
    eat_bits(state,12);
  }
  if (state->kval==-1) { // G4
    ret=decode_line_2d(state);
  } else if (state->kval==0) { // G3 1d
    ret=decode_line_1d(state);
  } else {
    if (next_bits(state,1)) { 
      eat_bits(state,1);
      ret=decode_line_1d(state);
    } else {
      eat_bits(state,1);
      ret=decode_line_2d(state);
    }
  }
  if (ret<0) { // error
    return ret;
  } else if (ret==1) { // File done
    return 1;
  }
  rle_decode(state->curline,outbuf,state->width);

  swap_lines(state);
  return 0;
}
