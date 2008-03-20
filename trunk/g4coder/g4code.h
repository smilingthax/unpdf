#ifndef _G4CODE_H
#define _G4CODE_H

#ifdef __cplusplus
extern "C" {
#endif

// have to return 0 on success, !=0 on error
typedef int (*WRITEFUNC)(void *user,unsigned char *buf,int len);
typedef int (*READFUNC)(void *user,unsigned char *buf,int len);

typedef struct G4STATE {
  READFUNC read;
  WRITEFUNC write;
  int width;
  int kval;
  void *user_read,*user_write;
  int *lastline,*curline;
  int lines_done,bitpos;
  unsigned int bitbuf;
} G4STATE;

// kval==-1 means G4-code, kval=0 means G3 1dim, kval>0 G3 2dim with K=>kval
// width<=0 means default (1728)
G4STATE *init_g4_read(int kval,int width,READFUNC rf,void *user_read);
G4STATE *init_g4_write(int kval,int width,WRITEFUNC wf,void *user_write);
void restart_g4(G4STATE *state);
void free_g4(G4STATE *state);

// The following functions encode/decode one line of 
// return 0 on success
//        1 on End-Of-File
//       <0 on Error
// >inbuf resp. >outbuf have to be ceil(width/8) bytes big
//
// When the image is done, call encode_g4 once more with >inbuf==NULL to 
// finish up the stream
int encode_g4(G4STATE *state,const unsigned char *inbuf);
// maybe we read 1 byte too much!
int decode_g4(G4STATE *state,unsigned char *outbuf);

#define ERR_INVALID_ARGUMENT 1
#define ERR_READ             2
#define ERR_WRITE            3
#define ERR_UNKNOWN_CODE     4
#define ERR_WRONG_CODE       5

#ifdef __cplusplus
};
#endif

#endif
