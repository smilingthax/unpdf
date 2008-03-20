#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "lzwcode.h"

// warnings
#include <stdio.h>

#define LZW_CLEAR 256
#define LZW_END   257
#define LZW_START 258
#define LZW_MINBITS  9
#define LZW_MAXBITS 12 // max 12 because of table=32 bit
#define LZW_HASHSIZE 9001  // at least 1<<MAXBITS, should be prime 

// accessors to table[]-values
#define NEXTBYTE(a)   ((a)&0xff)
#define PREFIXCODE(a) ((a>>8)&0xfff)
#define CODE(a)       ((a>>20)&0xfff) // encode only
#define MAKETABLE(code,prefixcode,nextbyte) ( (code<<20)|(prefixcode<<8)|(nextbyte) ) // for decode: code=0
// hash func; 
#define HASH(prefixcode,nextbyte) ( (((prefixcode<<8)|nextbyte)<<11)%LZW_HASHSIZE )

LZWSTATE *init_lzw(int earlychange,READFUNC rf,WRITEFUNC wf,void *user_read,void *user_write,int tablesize,unsigned char *stack)
{
  LZWSTATE *ret;

  if (earlychange<0) {
    earlychange=1; // default
  }

  ret=malloc(sizeof(LZWSTATE));
  if (!ret) {
    return 0;
  }

  ret->read=rf;
  ret->write=wf;
  ret->user_read=user_read;
  ret->user_write=user_write;

  ret->earlychange=earlychange;

  ret->table=malloc(tablesize*sizeof(unsigned int));
  if (!ret->table) {
    free(ret);
    return NULL;
  }
  ret->stackend=ret->stackptr=stack+(1<<LZW_MAXBITS); // this is tricky!

  restart_lzw(ret);
  return ret;
}

LZWSTATE *init_lzw_read(int earlychange,READFUNC rf,void *user_read) 
{
  unsigned char *stack;
  assert(rf);
  if (!rf) {
    return 0;
  }
  stack=malloc((1<<LZW_MAXBITS)*sizeof(unsigned char));
  if (!stack) {
    return NULL;
  }
  return init_lzw(earlychange,rf,NULL,user_read,NULL,1<<LZW_MAXBITS,stack);
}

LZWSTATE *init_lzw_write(int earlychange,WRITEFUNC wf,void *user_write) 
{
  assert(wf);
  if (!wf) {
    return 0;
  }
  return init_lzw(earlychange,NULL,wf,NULL,user_write,LZW_HASHSIZE,NULL);
}

void restart_lzw(LZWSTATE *state)
{
  assert(state);
  if (state) {
    state->numcodes=LZW_START;
    state->codebits=LZW_MINBITS;
    state->prefix=-1; // no prefix / clear table
    state->stackptr=state->stackend;

    state->bitbuf=0;
    state->bitpos=0;
  }
}

void free_lzw(LZWSTATE *state) 
{
  if (state) {
    free(state->stackend-(1<<LZW_MAXBITS)); // look at init_lzw !
    free(state->table);
    free(state);
  }
}

// helper
static int readbits(LZWSTATE *state)
{
  int ret,iA;
  unsigned char buf[4];

  if (state->bitpos<state->codebits) { // ensure enough bits
    int num=(state->codebits-state->bitpos+7)/8;
    ret=(*state->read)(state->user_read,buf,num);
    if (ret) {
      return -1;
    }
    for (iA=0;iA<num;iA++) {
      state->bitbuf|=buf[iA]<<(24-state->bitpos);
      state->bitpos+=8;
    }
  }
  state->bitpos-=state->codebits;
  ret=state->bitbuf>>(32-state->codebits);
  state->bitbuf<<=state->codebits;
  return ret;
}

static int writecode(LZWSTATE *state,unsigned int code)
{
  unsigned char buf[4];
  int iA=0;

  state->bitbuf|=code<<(32-state->bitpos-state->codebits);
  state->bitpos+=state->codebits;
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

static int writeflush(LZWSTATE *state)
{
  unsigned char c;

  assert(state->bitpos<8);
  if (!state->bitpos) {
    return 0;
  }
  c=state->bitbuf>>24;
  state->bitbuf=0;
  state->bitpos=0;
  return (*state->write)(state->user_write,&c,1);
}

// -> encode
// tries to append >nextbyte to >prefixcode.
// if a matching code is found: this is the new ("longer") prefixcode (>returned)
// otherwise a new code is created (state->numcodes resp. state->numcodes-1)
static inline int find_add_hash(LZWSTATE *state,int prefixcode,unsigned char nextbyte)
{
  unsigned int hash=HASH(prefixcode,nextbyte);

  while (1) {
    unsigned int ret=state->table[hash];
    if (!ret) { // empty entry
      break;
    }
    if ( (PREFIXCODE(ret)==prefixcode)&&(NEXTBYTE(ret)==nextbyte) ) { // found
      return CODE(ret);
    }
    hash=(hash+1)%LZW_HASHSIZE;
  }
  // not found: add entry
  state->table[hash]=MAKETABLE(state->numcodes,prefixcode,nextbyte);
  state->numcodes++;
  return -1;
}

// TODO: check errors from writecode
int encode_lzw(LZWSTATE *state,unsigned char *buf,int len)
{
  assert(state);
  assert(len>=0);
  if (!buf) { // finish up the stream
    if (state->prefix>=0) { // the current prefixcode won't become any longer
      writecode(state,state->prefix);
    }
    // TODO? empty stream:  LZW_CLEAR LZW_END
    writecode(state,LZW_END);
    writeflush(state);
    return 0;
  }
  while (len>0) {
    if (state->prefix==-1) { // begin / clear table
      writecode(state,LZW_CLEAR);
      memset(state->table,0,LZW_HASHSIZE*sizeof(unsigned int));
      state->numcodes=LZW_START;
      state->codebits=LZW_MINBITS;
      state->prefix=*buf;
      len--;
      buf++;
    }
    // here we go: find prefixcode
    for (;len>0;len--,buf++) {
      int code=find_add_hash(state,state->prefix,*buf);
      if (code==-1) { // no longer prefix found; new code assigned
        writecode(state,state->prefix);
        state->prefix=*buf; // set new prefix to current char

        if ( ((state->numcodes-1)==(1<<state->codebits)-state->earlychange-1)&&
             (state->codebits==LZW_MAXBITS) ) {
          state->prefix=-1; // clear table (one early, so we don't "have to" increase >codebits)
          break;
        } else if ((state->numcodes-1)==(1<<state->codebits)-state->earlychange) {
          state->codebits++;
        }
      } else {
        state->prefix=code;
      }
    }
  }
  return 0;
}

int decode_lzw(LZWSTATE *state,unsigned char *buf,int len)
{
  int outlen=0;
  assert(state);
  assert(len>=0);

  while (len>0) {
    // first empty the stack
    const int stacklen=state->stackend-state->stackptr;
    if (stacklen>0) {
      if (len<stacklen) {
        memcpy(buf,state->stackptr,len*sizeof(char));
        state->stackptr+=len;
        return 0;
      } else {
        memcpy(buf,state->stackptr,stacklen*sizeof(char));
        outlen+=stacklen;
        len-=stacklen;
        buf+=stacklen;
        state->stackptr=state->stackend;
        continue; // check for len==0;
      }
    }
    // decode next code
    int code=readbits(state);
    if (code<0) {
      return -1; // read error
    } else if (code==LZW_CLEAR) {
      state->numcodes=LZW_START;
      state->codebits=LZW_MINBITS;
      state->prefix=-1;
    } else if (code==LZW_END) {
      return 1+outlen; // done
    } else if (code<256) { // not in table
      *buf=code;
      buf++;
      len--;
      outlen++;
      if (state->prefix>=0) {
        state->table[state->numcodes++]=MAKETABLE(0,state->prefix,code);
      }
      state->prefix=code;
    } else if (code<state->numcodes) {
      int scode=code;
      assert(state->prefix>=0); 
      // push on stack to reverse 
      while (code>=256) {
        *--state->stackptr=NEXTBYTE(state->table[code]);
        code=PREFIXCODE(state->table[code]);
      }
      *--state->stackptr=code;
      // add to table
      state->table[state->numcodes++]=MAKETABLE(0,state->prefix,code);
      state->prefix=scode;
    } else if (code==state->numcodes) {
      if (state->prefix<0) {
        return -2; // invalid code, a <256 code is required first
      } 
      code=state->prefix;
      assert(state->stackptr==state->stackend); // the stack is empty!
      --state->stackptr; // will be filled later: first char==last char
      while (code>=256) {
        *--state->stackptr=NEXTBYTE(state->table[code]);
        code=PREFIXCODE(state->table[code]);
      }
      *--state->stackptr=code;
      state->stackend[-1]=code;
      state->table[state->numcodes]=MAKETABLE(0,state->prefix,code);
      state->prefix=state->numcodes++;
    } else {
      return -2; // invalid code
    }
    if (state->numcodes==(1<<state->codebits)-state->earlychange) {
      if (state->codebits==LZW_MAXBITS) {
        // just keep codebits
#if 1  // TODO? encode: as long as no high code used (->verbatim coding) there is no need for reset (Adobe PDFLib 5.0 does it!)
        state->numcodes--;
#else
        code=readbits(state);
        if (code<0) {
          return -1; // read error
        } else if (code==LZW_CLEAR) {
          fprintf(stderr,"Warning: overfull table\n");
          state->numcodes=LZW_START;
          state->codebits=LZW_MINBITS;
          state->prefix=-1;
        } else if (code==LZW_END) {
          fprintf(stderr,"Warning: overfull table\n");
          return 1+outlen; // done
        } else {
          fprintf(stderr,"Warning: over2full table: %d\n",code);
//          return -3; // table full
        }
#endif
      } else {
        state->codebits++;
      }
    }
  }
  return 0;
}
