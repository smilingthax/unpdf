#ifndef _TOOLS_H
#define _TOOLS_H
#include <stdarg.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inline char *a_vsprintf(const char *fmt,va_list ap) // {{{
{
  char *str,*tmp;
  int len=150,need;
  va_list va;
  
  str=(char *)malloc(len);
  if (!str) {
    throw std::bad_alloc();
  }
  while (1) {
    va_copy(va,ap);
    need=vsnprintf(str,len,fmt,va);
    va_end(va);
    if (need==-1) {
      len+=100;
    } else if (need>=len) {
      len=need+1;
    } else {
      break;
    }
    tmp=(char *)realloc(str,len);
    if (!tmp) {
      free(str);
      throw std::bad_alloc();
    }
    str=tmp;
  }
  return str;
}
// }}}

inline char *a_sprintf(const char *fmt,...) // {{{
{
  va_list va;
  char *ret;

  va_start(va,fmt);
  ret=a_vsprintf(fmt,va);
  va_end(va);
 
  return ret;
}
// }}}

inline std::string s_vsprintf(const char *fmt,va_list ap) // {{{
{
  std::string str;
  int need;
  va_list va;

  str.resize(150,0);
  
  while (1) {
    va_copy(va,ap);
    need=vsnprintf(&str[0],str.size(),fmt,va);
    va_end(va);
    if (need==-1) {
      str.resize(str.size()+100,0);
    } else if (need>=(int)str.size()) {
      str.resize(need+1,0);
    } else {
      str.resize(need);
      break;
    }
  }
  return str;
}
// }}}

inline std::string s_sprintf(const char *fmt,...) // {{{
{
  va_list va;
  std::string ret;

  va_start(va,fmt);
  ret=s_vsprintf(fmt,va);
  va_end(va);
 
  return ret;
}
// }}}

inline char *a_strtr(const char *string,const char **trFrom,const char **trTo) // {{{
{ 
  int iA,iB,len=0,startDelete=-1;
  char *ret=NULL;
 
  for (iB=0;trFrom[iB];iB++) {
    if (!trTo[iB]) {
      startDelete=iB;
      break;
    }
  }
  if (!trFrom[iB]) { // TRICK: now we have length(trFrom)
    startDelete=iB;
  }
  for (iA=0;string[iA];iA++) {
    for (iB=0;trFrom[iB];iB++) {
      if (strncmp(string+iA,trFrom[iB],strlen(trFrom[iB]))==0) {
        if (iB<startDelete) {
          len+=strlen(trTo[iB]);
        }
        iA+=strlen(trFrom[iB])-1;
        break;
      }
    }
    if (!trFrom[iB]) { // untranslated
      len++;
    }
  }
  ret=(char *)malloc(len+1);
  if (!ret) {
    throw std::bad_alloc();
  }
  len=0;
  for (iA=0;string[iA];iA++) {
    for (iB=0;trFrom[iB];iB++) {
      if (strncmp(string+iA,trFrom[iB],strlen(trFrom[iB]))==0) {
        if (iB<startDelete) {
          strcpy(ret+len,trTo[iB]);
          len+=strlen(trTo[iB]);
        }
        iA+=strlen(trFrom[iB])-1;
        break;
      }
    }
    if (!trFrom[iB]) { // untranslated
      ret[len]=string[iA];
      len++;
    }
  }
  ret[len]=0;
  return ret;
}
// }}}

inline std::string s_strtr(const std::string &string,const char **trFrom,const char **trTo) // {{{
{ 
  int iA,iB,startDelete=-1;
  std::string ret;
 
  for (iB=0;trFrom[iB];iB++) {
    if (!trTo[iB]) {
      startDelete=iB;
      break;
    }
  }
  if (!trFrom[iB]) { // TRICK: now we have length(trFrom)
    startDelete=iB;
  }
  const int len=string.size();
  ret.reserve(len);
  for (iA=0;iA<len;iA++) {
    for (iB=0;trFrom[iB];iB++) {
      if (string.compare(iA,strlen(trFrom[iB]),trFrom[iB])==0) {
        if (iB<startDelete) {
          ret.append(trTo[iB]);
        }
        iA+=strlen(trFrom[iB])-1;
        break;
      }
    }
    if (!trFrom[iB]) { // untranslated
      ret.push_back(string[iA]);
    }
  }
  return ret;
}
// }}}

inline char *a_pureascii(const char *string)
{
  static const char *trFrom[]={ "\xe4", "\xf6", "\xfc", "\xc4", "\xd6", "\xdc", "\xdf"," ",NULL}, // latin1
                    *trTo[]  ={   "ae",   "oe",   "ue",   "Ae",   "Oe",   "Ue",   "ss",NULL};
  return a_strtr(string,trFrom,trTo);
}

inline std::string to_hex(const unsigned char *buf,int len) // {{{
{
  static const char hex[]="0123456789abcdef";
  std::string ret(2*len,0);
  for (int iA=0;iA<len;iA++) {
    ret[2*iA]=hex[buf[iA]>>4];
    ret[2*iA+1]=hex[buf[iA]&0xf];
  }
  return ret;
}
// }}}

#endif
