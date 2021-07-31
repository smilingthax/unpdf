#define _LARGEFILE64_SOURCE
#include "filesystem.h"
#include <errno.h>
#include <sys/stat.h>
#ifndef _WIN32
#  include <sys/statvfs.h>
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

#ifdef _WIN32
#include <windef.h>   // HWND
#include <winbase.h>  // FormatMessage

std::string Win32_except::_formatMsg(DWORD code, const char *cmd, const char *extra) // {{{
{
  // assert(cmd);
  char str[256];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, str, sizeof(str), NULL);
  if (extra) {
    return std::string(cmd ? cmd : "") + "(" + extra + "): " + str;
  } else if (cmd) {
    return std::string(cmd) + ": " + str;
  } else {
    return str;
  }
}
// }}}
#endif

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

void FS::create_dir(const string &dirname,bool skip_existing,unsigned int mode) // {{{
{
#ifdef _WIN32
  const int res=mkdir(dirname.c_str());
#else
  const int res=mkdir(dirname.c_str(),mode);
#endif
  if (res==-1) {
    if ( (!skip_existing)||(errno!=EEXIST) ) {
      throw FS_except(errno,"mkdir",dirname.c_str());
    }
    if (!is_dir(dirname)) {
      throw FS_except(ENOTDIR,"mkdir",dirname.c_str());
    }
  }
}
// }}}

void FS::create_dirs(const string &dirname,unsigned int mode) // {{{
{
  if (dirname.empty()) {
    throw FS_except(ENOENT,"mkdir",dirname.c_str());
  }

  int ret=0;

  // optimization: try full path (create only last component) first
  // - this also prevents throwing, when if target dir exists
#ifdef _WIN32
  ret=mkdir(dirname.c_str());
#else
  ret=mkdir(dirname.c_str(),mode);
#endif
  if (ret==0) {
    return;
  } else if (errno==EEXIST) {
    if (!is_dir(dirname)) {
      throw FS_except(ENOTDIR,"mkdir",dirname.c_str());
    }
    return;
  }

  size_t start=dirname.find_first_not_of('/'),pos;

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

#if (defined(_LARGEFILE64_SOURCE) && !defined(__x86_64__) && !defined(__ppc64__)) || defined(_WIN32)
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

#ifdef _WIN32
#include <fileapi.h>
#include <errhandlingapi.h>

FS::dstat_t FS::get_diskstat(const std::string &path,bool rootspace) // {{{
{
  ULARGE_INTEGER free_bytes,total_bytes,rootfree_bytes;
  if (!GetDiskFreeSpaceExA(path.c_str(),&free_bytes,&total_bytes,&rootfree_bytes)) {
    throw Win32_except(GetLastError(),"GetDiskFreeSpaceEx()");
  }
  DWORD flags;
  if (!GetVolumeInformationA(path.c_str(),NULL,0,NULL,NULL,&flags,NULL,0)) {
    throw Win32_except(GetLastError(),"GetVolumeInformation()");
  }
  dstat_t ret;
  ret.sum_space=total_bytes.QuadPart;
  if (rootspace) {
    ret.free_space=rootfree_bytes.QuadPart;
  } else {
    ret.free_space=free_bytes.QuadPart;
  }
  ret.readonly=(flags & FILE_READ_ONLY_VOLUME);
  return ret;
}
// }}}
#else
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

#ifdef _WIN32
static inline int lstat(const char *pathname, struct stat *statbuf)
{
  return stat(pathname, statbuf);
}
#endif

int FS::remove_all(const std::string &path) // {{{
{
  int ret = 0;
  struct stat st;

  if (path.empty()) {
    return ret;
  } else if (lstat(path.c_str(), &st) == -1) {
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

      if (lstat(fullpath.c_str(), &st) == -1) {
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
// }}}


#include <fcntl.h>
#include <unistd.h>
#if _POSIX_MAPPED_FILES  // in unistd.h for POSIX systems that have mmap
#  include <sys/mman.h>
#else
#  include <stdlib.h>
#endif

struct unique_fd { // {{{
  unique_fd(int fd) : fd(fd) {}
  ~unique_fd() {
    close();
  }
  unique_fd(const unique_fd &) = delete;
  unique_fd &operator=(const unique_fd &) = delete;
  void close() {
    if (fd >= 0) {
      ::close(fd);
      fd = -1;
    }
  }
  operator int() {
    return fd;
  }
private:
  int fd;
};
// }}}

struct unique_unlink { // {{{
  unique_unlink(const std::string &name) : name(name) {}
  ~unique_unlink() {
    if (!name.empty()) {
      unlink(name.c_str());  // NOTE: do not throw in ctor...
    }
  }
  unique_unlink(const unique_fd &) = delete;
  unique_unlink &operator=(const unique_unlink &) = delete;
  void release() {
    name.clear();
  }
private:
  std::string name;
};
// }}}

struct unique_fd_unlink final { // {{{
  unique_fd_unlink(int fd, const std::string &name)
    : unlinker(fd >= 0 ? name : std::string{}), fd(fd)
  { }
  unique_fd_unlink(const unique_unlink &) = delete;
  unique_fd_unlink &operator=(const unique_fd_unlink &) = delete;
  void finish() {
    fd.close();
    unlinker.release();
  }
  operator int() {
    return fd;
  }
private:
  unique_unlink unlinker; // dtor must run *after* unique_fd!
  unique_fd fd;
};
// }}}

#if _POSIX_MAPPED_FILES
struct unique_mmap { // {{{
  unique_mmap(void *ptr, size_t len)
    : ptr(ptr), len(len)
  { }
  ~unique_mmap() {
    munmap(ptr, len);  // retval: dtor shall not throw ...
  }
  unique_mmap(const unique_mmap &) = delete;
  unique_mmap &operator=(const unique_mmap &) = delete;
  operator void *() {
    return ptr;
  }
private:
  void *ptr;
  size_t len;
};
// }}}
#endif

static void do_write(int fd, const char *buf, size_t len) // {{{
{
  size_t offset = 0;
  do {
    const ssize_t res = write(fd, buf + offset, len - offset);
    if (res < 0) {
      throw FS_except(errno, "write()");
    }
    offset += res;
  } while (offset < len);
}
// }}}

static int do_open_new(const std::string &name, int flags, mode_t mode, bool overwrite) // {{{
{
  flags |= O_CREAT | O_EXCL;
  const int fd = open(name.c_str(), flags, mode);
  if (fd == -1 && overwrite && errno == EEXIST) {
    // TODO(optim)? if "same file", return immediate success of copy_file ?
    if (unlink(name.c_str()) == -1) {
      throw FS_except(errno, "unlink", name.c_str());
    }
    return open(name.c_str(), flags, mode);
  }
  return fd;
}
// }}}

void FS::copy_file(const std::string &from, const std::string &to, bool overwrite) // {{{
{
  struct stat st;
#ifndef _WIN32
#  define _O_BINARY  0
#endif

  unique_fd ifd{open(from.c_str(), O_RDONLY | _O_BINARY)};
  if (ifd == -1) {
    throw FS_except(errno, "open", from.c_str());
  }

  if (fstat(ifd, &st) == -1) {
    throw FS_except(errno, "fstat", from.c_str());
  } else if (!S_ISREG(st.st_mode)) {
    throw FS_except(ENOTSUP, "FS::copy_file", from.c_str());
  }

  // TRICK: unlink + reopen the destination only after we have an open handle to the source,
  //        so we can still access the old contents, even when it was (e.g.) a symlink to the destination
  unique_fd_unlink ofd{do_open_new(to.c_str(), O_WRONLY | _O_BINARY, S_IWUSR, overwrite), to.c_str()}; // start with (0200 & ~umask), chmod later to real mode
  if (ofd == -1) {
    throw FS_except(errno, "open", to.c_str());
  }
#undef _O_BINARY

#ifdef _WIN32
  if (chmod(to.c_str(), st.st_mode) == -1) {
    throw FS_except(errno, "chmod", to.c_str());
  }
#else
  if (fchmod(ofd, st.st_mode) == -1) {
    throw FS_except(errno, "fchmod", to.c_str());
  }
#endif

#if _POSIX_MAPPED_FILES
  const off_t len = st.st_size;
  if (len > 0) {
    // TODO/FIXME: loop ("re-mmap" w/ incrementing offset [cave: offsets must be page aligned!!!])
    unique_mmap ptr{mmap(NULL, len, PROT_READ, MAP_SHARED, ifd, 0), (size_t)len};
    if (ptr == MAP_FAILED) {
      throw FS_except(errno, "mmap", from.c_str());
    }

    do_write(ofd, (const char *)(void *)ptr, len);
  }

#else
  std::vector<char> buf;
  buf.resize(16 * 1024); // 16k

  while (1) {
    const ssize_t len = read(ifd, buf.data(), buf.size());
    if (len == 0) {
      break;
    } else if (len < 0) {
      throw FS_except(errno, "read", from.c_str());
    }

    do_write(ofd, buf.data(), len);
  }
#endif

  ofd.finish();
}
// }}}

void FS::move_file(const std::string &from, const std::string &to, bool overwrite) // {{{
{
  struct stat st;

  // NOTE: we dereference (symbolic link) from
  if (stat(from.c_str(), &st) != 0) {
    throw FS_except(errno, "stat", from.c_str());
  } else if (S_ISDIR(st.st_mode)) {
    throw FS_except(EACCES, "FS::move_file", from.c_str());
  } else if (!S_ISREG(st.st_mode)) {
    throw FS_except(ENOTSUP, "FS::move_file", from.c_str());
  }

#ifdef _WIN32
  if (!MoveFileExA(from.c_str(), to.c_str(), (overwrite ? MOVEFILE_REPLACE_EXISTING : 0) | MOVEFILE_COPY_ALLOWED)) {
    throw Win32_except(GetLastError(), "MoveFileEx", (from + ", " + to).c_str());
  }
#else

  if (!overwrite && lstat(to.c_str(), &st) == 0) {
    throw FS_except(EEXIST, "FS::move_file", (from + ", " + to).c_str());
  }
  // NOTE: no need to check (overwrite && stat(to.c_str(), &st)) == 0 && S_ISDIR(st.st_mode)): rename *will* fail

  if (rename(from.c_str(), to.c_str()) == 0) {
    return;
  }

  if (errno != EXDEV) {
    throw FS_except(errno, "rename", (from + ", " + to).c_str());
  }

  // fallback to copy + unlink
  FS::copy_file(from, to, overwrite);  // CAVE: must work correctly even when from and to somehow point to the same file!

  // NOTE: this can never remove/invalidate the destination file, when from/to "point to the same file"
  if (unlink(from.c_str()) == -1) {
    fprintf(stderr, "FS::move_file failed to remove original file (%s): %s\n", from.c_str(), strerror(errno));  // TODO?
  }
#endif
}
// }}}

