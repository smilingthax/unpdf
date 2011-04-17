#ifndef _FTOOL_H
#define _FTOOL_H

#include <stdio.h>
#include <string>

std::string &s_fgets(FILE *f,std::string &ret,int maxlen=-1);

/*namespace File (?)
char *a_fgets(FILE *f)
{
}
*/

inline std::string s_fgets(FILE *f)
{
  std::string ret;
  s_fgets(f,ret);
  return ret;
}

#endif
