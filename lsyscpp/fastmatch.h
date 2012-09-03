#ifndef _FASTMATCH_H
#define _FASTMATCH_H

// (c) 2009-2010 Tobias Hoffmann, Dual-licensed: MIT/GPL.

/* Usage:

NEWMATCH
MATCH("text")
  c code ..
SUBMATCH("utext")
  if (?) return false; // will also work!
  c code ..
SUBMATCH_END("utext")
  c code; >_buf points behind "utext"
ELSE
  c code ...
ENDMATCH(name)
  
  Note: The match-texts MUST be sorted ascending!

  This will generate:
bool name(char *&_buf);  (to be exact: template<class T> bool name(T &_buf)) 
  resp:
template<typename UserData>
bool name(char *&_buf,UserData &user);

  To declare friendship (e.g. to access private UserData members) use:
  class UserData {
    ...
    MATCHFRIEND(name)
  };

  Advanced:
ELSE_NULL  sets _buf==NULL  
  The else-part can also be written directly after NEWMATCH, omitting the ELSE keyword.

CASE(strcmp(_buf,"text"))   is what MATCH("text") is an abbreviation for.
  the function must return <0, >0 or ==0, when _buf comes before, after, or matches "text", respectively.

*/

namespace {
template<int N> struct ssw_counter { enum { block=ssw_counter<N-1>::block, base=ssw_counter<N-1>::base }; };
template<int L,int R,int N> struct ssw_treecalc { enum { here=(L+R)/2, nextleft=(N<=here)?L:here+1, nextright=(N>here)?R:here,
                                                         left=(N==here)?L:ssw_treecalc<nextleft,nextright,N>::left,
                                                         right=(N==here)?R:ssw_treecalc<nextleft,nextright,N>::right }; };
template<int N> struct ssw_treecalc<N,N,N> { enum { left=N, right=N }; };
template<int N,int SIZE,typename Tstring,typename U> struct block_no { enum { belse=false }; };
}

#define NEWMATCH namespace { \
  template<> struct ssw_counter<__LINE__> { enum { block=__LINE__, base=__LINE__ }; }; \
  template <int SIZE,typename Tstring,typename U> struct block_no<__LINE__,SIZE,Tstring,U> { \
    enum { belse=true }; \
    inline static bool run(Tstring _buf,U user) { \
      if (block_no<__LINE__+SIZE+1,SIZE,Tstring,U>::belse) { \
        return block_no<(block_no<__LINE__+SIZE+1,SIZE,Tstring,U>::belse)?__LINE__+SIZE+1:__LINE__,SIZE,Tstring,U>::run(_buf,user); \
      } else {
#define CASE(fun) \
        return !belse; \
      } \
    } \
  }; \
  template <> struct ssw_counter<__LINE__> { enum { block=ssw_counter<__LINE__-1>::block+1, base=ssw_counter<__LINE__-1>::base }; }; \
  template <int SIZE,typename Tstring,typename U> struct block_no<ssw_counter<__LINE__>::block,SIZE,Tstring,U> { \
    enum { block=(int)ssw_counter<__LINE__>::block, base=ssw_counter<__LINE__>::base, belse=false, \
           left=ssw_treecalc<base+1,base+1+SIZE,block>::left, right=ssw_treecalc<base+1,base+1+SIZE,block>::right, \
           nextleft=(left<block)?(left+block)/2:base, nextright=(block+1<right)?(block+1+right)/2:base }; \
    inline static bool run(Tstring _buf,U user) { \
      const int _c=fun; \
      if (_c<0) { \
        return block_no<nextleft,SIZE,Tstring,U>::run(_buf,user); \
      } else if (_c>0) { \
        return block_no<nextright,SIZE,Tstring,U>::run(_buf,user); \
      } else {
#define MATCH(x) CASE(strcmp(_buf,x))
#define SUBMATCH(x) CASE(strncmp(_buf,x,strlen(x)))
#define SUBMATCH_END(x) CASE(strncmp(_buf,x,strlen(x))) _buf+=strlen(x);
#define ELSE \
        return !belse; \
      } \
    } \
  }; \
  template <> struct ssw_counter<__LINE__> { enum { block=ssw_counter<__LINE__-1>::block, base=ssw_counter<__LINE__-1>::base }; }; \
  template <int SIZE,typename Tstring,typename U> struct block_no<ssw_counter<__LINE__>::block+1,SIZE,Tstring,U> { \
    enum { belse=true }; \
    inline static bool run(Tstring _buf,U user) { \
      if (1) {
#define ELSE_NULL ELSE _buf=NULL;
#define ENDMATCH(name) \
        return !belse; \
      } \
    } \
  }; \
  template<> struct ssw_counter<__LINE__> { enum { block=__LINE__, base=ssw_counter<__LINE__-1>::base, \
                                                   numblocks=ssw_counter<__LINE__-1>::block-base, \
                                                   next=(numblocks>0)?base+1+numblocks/2:base }; }; \
  } /*namespace end*/ \
  template <typename T,typename U> \
  inline bool name (T &buf,U &user) { \
    return block_no<ssw_counter<__LINE__>::next,ssw_counter<__LINE__>::numblocks,T &,U &>::run(buf,user); \
  } \
  template <typename T> \
  inline bool name (T &buf) { \
    return block_no<ssw_counter<__LINE__>::next,ssw_counter<__LINE__>::numblocks,T &,void *>::run(buf,NULL); \
  }
#define MATCHFRIEND(name) template<int N,int SIZE,typename Tstring,typename U> friend struct ::block_no;

#endif
