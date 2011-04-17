#include "ftool.h"
#include "filesystem.h"
#include <errno.h>

using namespace std;

// ?? GNU: ssize_t getline(char **line,size_t *size,FILE *f)
#if 0
// problem with \0 !
string &s_fgets(FILE *f,string &ret,int maxlen)
{
  ret.clear();
#define BUFLEN 128
  int oldlen=0;
  do {
    ret.resize(oldlen+BUFLEN);
    if (!fgets(&ret[oldlen],BUFLEN,f)) { // error or eof
      if (feof(f)) {
        break;
      }
      throw FS_except(errno);
    }
    oldlen+=strlen(ret.data()+oldlen); // only data() required!
    if ( (maxlen!=-1)&&(oldlen>=maxlen) ) {
      throw FS_except(EFBIG);
    }
  } while (ret[oldlen-1]!='\n');
  ret.resize(oldlen);
  return ret;
#undef BUFLEN
}
#else

string &s_fgets(FILE *f,string &ret,int maxlen)
{
  ret.clear();
  int c;
  while ((c=fgetc(f))!=EOF) {
    ret.push_back(c);
    if (c=='\n') {
      return ret; 
    }
    if ( (maxlen!=-1)&&((int)ret.size()>=maxlen) ) {
      throw FS_except(EFBIG);
    }
  }
  if (feof(f)) {
    return ret;
  }
  throw FS_except(errno);
}

#endif

