#ifndef _LZWCODE_H
#define _LZWCODE_H

#ifdef __cplusplus
extern "C" {
#endif

// have to return 0 on success, !=0 on error
typedef int (*WRITEFUNC)(void *user,unsigned char *buf,int len);
typedef int (*READFUNC)(void *user,unsigned char *buf,int len);

typedef struct LZWSTATE {
  READFUNC read;
  WRITEFUNC write;
  void *user_read,*user_write;

  int earlychange;

  int numcodes; // currently used codes
  int codebits; // currently used bits
  int prefix; // current prefix (encoding) / last code (decoding)
  unsigned int *table; // encoding: hash-table (code[12bit],prefixcode[12bit],nextbyte)[hash(prefixcode,nextbyte)]
                       // decoding: symbol-table (prefixcode,nextbyte)[code]
  unsigned char *stackend,*stackptr; // for decoding.

  int bitpos;
  unsigned int bitbuf;
} LZWSTATE;

LZWSTATE *init_lzw_read(int earlychange,READFUNC rf,void *user_read);
LZWSTATE *init_lzw_write(int earlychange,WRITEFUNC wf,void *user_write);
void restart_lzw(LZWSTATE *state);
void free_lzw(LZWSTATE *state);

// return 0 on success, <0 on error
// to finish the stream: call once with >buf==NULL
int encode_lzw(LZWSTATE *state,unsigned char *buf,int len);
// returns 1+len(really decoded) on EOD 
int decode_lzw(LZWSTATE *state,unsigned char *buf,int len);
// TODO: error: "Warning: EOD missing, EOF came first\n"

#ifdef __cplusplus
};
#endif

#endif
