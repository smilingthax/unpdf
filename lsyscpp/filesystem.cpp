#define _LARGEFILE64_SOURCE
#include "filesystem.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

FS_except::FS_except(int errnum) throw() : errnum(errnum)
{
  errtext=string(strerror(errnum));
}

string FS::cwd() // {{{
{
  char *tmp=getcwd(NULL,0);
  if (!tmp) {
    throw FS_except(errno);
  }
  string ret(tmp);
  free(tmp);
  return ret;
}
// }}}

string FS::abspath(const string &filename) // {{{
{
  if (is_abspath(filename)) {
    return filename;
  }
  return joinPath(cwd(),filename);
}
// }}}

string FS::basename(const string &filename) // {{{
{
  size_t lpos=filename.find_last_of('/',filename.find_last_not_of('/'));
  if (lpos==string::npos) {
    lpos=0;
  } else {
    lpos++;
  }
  string ret(filename,lpos,string::npos);

  if ( (ret.empty())&&(filename[0]=='/') ) {
    return string("/");
  }
  return ret;
}
// }}}

string FS::dirname(const string &filename) // {{{
{
  string ret(filename,0,filename.find_last_of('/',filename.find_last_not_of('/')));
  ret.resize(ret.find_last_not_of('/')+1);

  if ( (ret.empty())&&(filename[0]=='/') ) {
    return string("/");
  }
  return ret;
}
// }}}

bool FS::exists(const string &filename) // {{{
{
  struct stat st;
  if (filename.empty()) {
    return false;
  }
  return stat(filename.c_str(),&st)==0;
}
// }}}

bool FS::is_file(const std::string &path) // {{{
{
  struct stat st;
  if (path.empty()) {
    return false;
  }
  return (stat(path.c_str(),&st)==0)&&(S_ISREG(st.st_mode));
}
// }}}

bool FS::is_dir(const std::string &path) // {{{
{
  struct stat st;
  if (path.empty()) {
    return false;
  }
  return (stat(path.c_str(),&st)==0)&&(S_ISDIR(st.st_mode));
}
// }}}

void FS::create_dir(const string &dirname) // {{{
{
  int res=mkdir(dirname.c_str(),0777);
  if (res==-1) {
    throw FS_except(errno);
  }
}
// }}}

string FS::joinPath(const string &a1,const string &a2) // {{{
{
  string ret(a1,0,a1.find_last_not_of('/')+1);
  ret.push_back('/');
  if (!a2.empty()) {
    ret.append(a2,a2.find_first_not_of('/'),string::npos);
  }
  return ret;
}
// }}}

#ifdef _LARGEFILE64_SOURCE
string FS::humanreadable_size(off64_t size) // {{{
{
  char tmp[40];
  size/=1024;
  if (size>1024) {
    size/=1024;
    if (size>1024) {
      size/=1024;
      snprintf(tmp,35,"%dG",(int)size);
    } else {
      snprintf(tmp,35,"%dM",(int)size);
    }
  } else {
    if (size<1) {
      size=1;
    }
    snprintf(tmp,35,"%dK",(int)size);
  }
  return string(tmp);
}
// }}}
#endif

FS::dstat_t FS::get_diskstat(const std::string &path,bool rootspace) // {{{
{
  struct statvfs svs;
  int res=statvfs(path.c_str(),&svs);
  if (res!=0) {
    throw FS_except(errno);
  }
  dstat_t ret;
  ret.sum_space=(long long)svs.f_frsize*svs.f_blocks;
  if (rootspace) {
    ret.free_space=(long long)svs.f_frsize*svs.f_bfree;
  } else {
    ret.free_space=(long long)svs.f_frsize*svs.f_bavail;
  }
  /*
  long sum_files,free_files_root,free_files_nonroot;
  sum_files=svs.f_files;
  free_files_root=svs.f_ffree;
  free_files_nonroot=svs.f_favail;
  */
  return ret;
}
// }}}

string FS::humanreadable_size(off_t size) // {{{
{
  char tmp[40];
  size/=1024;
  if (size>1024) {
    size/=1024;
    if (size>1024) {
      size/=1024;
      snprintf(tmp,35,"%dG",(int)size);
    } else {
      snprintf(tmp,35,"%dM",(int)size);
    }
  } else {
    if (size<1) {
      size=1;
    }
    snprintf(tmp,35,"%dK",(int)size);
  }
  return string(tmp);
}
// }}}

// TODO? does not distinguish between 'text.' and 'text'
pair<string,string> FS::extension(const string &filename) // {{{
{
  const size_t pos=filename.rfind('.');
  if (pos==string::npos) {
    return make_pair(filename,string());
  }
  return make_pair(filename.substr(0,pos),filename.substr(pos+1));
}
// }}}
