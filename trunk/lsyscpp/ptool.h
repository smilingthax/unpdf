#ifndef _PTOOL_H
#define _PTOOL_H

#include <string>
#include <vector>
#include <string.h>

// NOTE: none of these returns empty strings

// any of >delims
void str_split(std::vector<std::string> &ret,const std::string &buf,const std::string &delims) { // {{{
  size_t start=0,pos;

  while ((pos=buf.find_first_of(delims,start))!=std::string::npos) {
    if (pos>start) {
      ret.push_back(buf.substr(start,pos-start));
    }
    start=pos+1;
  }
  if (start<buf.size()) {
    ret.push_back(buf.substr(start));
  }
}
// }}}

void str_split(std::vector<std::string> &ret,const char *buf,const char *delims) { // {{{
  buf+=strspn(buf,delims);
  while (*buf) {
    size_t pos=strcspn(buf,delims);

    ret.push_back(std::string(buf,pos));
    buf+=pos;
    buf+=strspn(buf,delims);
  }
}
// }}}

// exact >delim
void str_explode(std::vector<std::string> &ret,const std::string &buf,const std::string &delim) { // {{{
  size_t start=0,pos;

  while ((pos=buf.find(delim,start))!=std::string::npos) {
    if (pos>start) {
      ret.push_back(buf.substr(start,pos-start));
    }
    start=pos+delim.size();
  }
  if (start<buf.size()) {
    ret.push_back(buf.substr(start));
  }
}
// }}}

#endif
