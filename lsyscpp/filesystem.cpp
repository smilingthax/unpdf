#define _LARGEFILE64_SOURCE
#include "filesystem.h"
#include <errno.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/statvfs.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// TODO?! ( defined(_WIN32)||defined(__WIN32__) )&&(!defined(__CYGWIN__)

using namespace std;

FS_except::FS_except(int errnum,const char *cmd,const char *extra) throw() : errnum(errnum)
{
  if (extra) {
    errtext=string(cmd ? cmd : "") + "(" + extra + "): " + strerror(errnum);
  } else if (cmd) {
    errtext=string(cmd) + ": " + strerror(errnum);
  } else {
    errtext=string(strerror(errnum));
  }
}

// TODO: ? only win32, linux, os x
// win32: #include <direct.h>
string FS::cwd() // {{{
{
  char *tmp=getcwd(NULL,0);  // win32: _getcwd
  if (!tmp) {
    throw FS_except(errno,"getcwd()");
  }
  string ret(tmp);
  free(tmp);
  return ret;
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

void FS::create_dir(const string &dirname,unsigned int mode) // {{{
{
#ifdef _WIN32
  int res=mkdir(dirname.c_str());
#else
  int res=mkdir(dirname.c_str(),mode);
#endif
  if (res==-1) {
    throw FS_except(errno,"mkdir",dirname.c_str());
  }
}
// }}}

void FS::create_dirs(const string &dirname,unsigned int mode) // {{{
{
  if (dirname.empty()) {
    throw FS_except(ENOENT,"mkdir",dirname.c_str());
  }

  size_t start=dirname.find_first_not_of('/'),pos;
  int ret=0;

  while ((pos=dirname.find_first_of('/',start))!=std::string::npos) {
    // TODO? windows: drive letter (C:\...) / UNC path (\\server\...)
    if (!is_special_dot(dirname.substr(start,pos-start))) {
#ifdef _WIN32
      ret=mkdir(dirname.substr(0,pos).c_str());
#else
      ret=mkdir(dirname.substr(0,pos).c_str(),mode);
#endif
    }
    start=dirname.find_first_not_of('/',pos+1);
  }
  if ( (start<dirname.size())&&(!is_special_dot(dirname.substr(start))) ) {
#ifdef _WIN32
    ret=mkdir(dirname.c_str());
#else
    ret=mkdir(dirname.c_str(),mode);
#endif
  }
  if (ret==-1) {
    throw FS_except(errno,"mkdir",dirname.c_str());
  }
}
// }}}

string FS::joinPath(const string &a1,const string &a2) // {{{
{
  if (is_abspath(a2)) { // i.e. starts with '/'
    // TODO? simplify "//path" ?:  return string(a2,a2.find_first_not_of('/'));
    return a2;
  }
  string ret(a1,0,a1.find_last_not_of('/')+1);
  ret.push_back('/');
  ret.append(a2);
  return ret;
}
// }}}

string FS::joinPathRel(const string &a1,const string &a2) // {{{
{
  string ret(a1,0,a1.find_last_not_of('/')+1);
  ret.push_back('/');
  if (is_abspath(a2)) { // i.e. starts with '/'
     ret.append(a2,a2.find_first_not_of('/'), string::npos);
  } else {
     ret.append(a2);
  }
  return ret;
}
// }}}

#if defined(_LARGEFILE64_SOURCE) && !defined(__x86_64__) && !defined(__ppc64__)
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

#ifndef _WIN32  // for now
FS::dstat_t FS::get_diskstat(const std::string &path,bool rootspace) // {{{
{
  struct statvfs svs;
  int res=statvfs(path.c_str(),&svs);
  if (res!=0) {
    throw FS_except(errno,"statvfs",path.c_str());
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
#endif

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


#include <dirent.h>
#include <vector>
#include <memory>

//#define NOOP_REMOVE

struct dir_stack_t {
  dir_stack_t(DIR *d, size_t pathpos) : d(d), pathpos(pathpos) {}
  struct closedir_deleter {
    void operator()(DIR *d) {
      closedir(d);
    }
  };
  std::unique_ptr<DIR, closedir_deleter> d;
  size_t pathpos;
};

int FS::remove_all(const std::string &path)
{
  int ret = 0;
  struct stat st;

  if (path.empty()) {
    return ret;
  } else if (lstat(path.c_str(), &st) == -1) {  // TODO/FIXME: not on _WIN32 -> just stat ...?
    return ret;
  }

  if (!S_ISDIR(st.st_mode)) {
#ifdef NOOP_REMOVE
    printf("noop unlink(%s)\n", path.c_str());
    ret++;
#else
    if (unlink(path.c_str()) != -1) {
      ret++;
    }
#endif
    return ret;
  }

  std::string fullpath = path;
  if (fullpath.back() != '/') {
    fullpath.push_back('/');
  }

  std::vector<dir_stack_t> stack;

  DIR *d = opendir(fullpath.c_str());
  if (!d) {
    return ret;
  }
  stack.emplace_back(d, fullpath.size());

  do {
    DIR *d = stack.back().d.get();
    size_t pathpos = stack.back().pathpos;
    struct dirent *de;

    while ((de = readdir(d)) != NULL) {
      if (is_special_dot(de->d_name)) {
        continue;
      }

      fullpath.resize(pathpos);
      fullpath.append(de->d_name);

      if (lstat(fullpath.c_str(), &st) == -1) {  // not on _WIN32 -> just stat ...?
        continue;
      }
      if (S_ISDIR(st.st_mode)) {
        fullpath.push_back('/');

        d = opendir(fullpath.c_str());
        if (!d) {
          d = stack.back().d.get();
          continue;
        }
        stack.emplace_back(d, fullpath.size());

        // descend [d already updated]  (alt: continue/goto outer loop)
        pathpos = stack.back().pathpos;

      } else {
#ifdef NOOP_REMOVE
        printf("noop unlink(%s)\n", fullpath.c_str());
        ret++;
#else
        if (unlink(fullpath.c_str()) != -1) {
          ret++;
        }
#endif
      }
    }
    stack.pop_back();
    fullpath.resize(pathpos);

#ifdef NOOP_REMOVE
    printf("noop rmdir(%s)\n", fullpath.c_str());
    ret++;
#else
    if (rmdir(fullpath.c_str()) != -1) {
      ret++;
    }
#endif
  } while (!stack.empty());

  return ret;
}

