#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <string>
#include <stdexcept>
#include <unistd.h>

class FS_except : public std::exception {
public:
  explicit FS_except(int errnum,const char *cmd=NULL,const char *extra=NULL) throw();
  ~FS_except() throw() {}
  const char *what() const throw() { return errtext.c_str(); }
  int err_no() const throw() { return errnum; }
private:
  std::string errtext;
  int errnum;
};

#ifdef _WIN32
#include <minwindef.h>

struct Win32_except : public std::runtime_error {
  Win32_except(DWORD code, const char *cmd=NULL,const char *extra=NULL)
    : std::runtime_error(_formatMsg(code, cmd, extra))
  { }
  static std::string _formatMsg(DWORD code, const char *cmd=NULL, const char *extra=NULL);
};
#endif

namespace FS {
  std::string cwd();
  std::string dirname(const std::string &filename);
  std::string basename(const std::string &filename);
  bool exists(const std::string &filename);
  bool is_file(const std::string &path);
  bool is_dir(const std::string &path);
  void create_dir(const std::string &dirname,unsigned int mode=0755);  // mode is ignored on win32
  void create_dirs(const std::string &dirname,unsigned int mode=0755);  // mode is ignored on win32; does not support C:\... or \\server\...
  std::string joinPath(const std::string &a1,const std::string &a2); // will return only a2, if absolute.
  std::string joinPathRel(const std::string &a1,const std::string &a2); // will always concatenate
  std::pair<std::string,std::string> extension(const std::string &filename);

  int remove_all(const std::string &path); // returns number of removed entries

  inline std::string abspath(const std::string &filename) { return joinPath(cwd(),filename); }

  inline bool is_special_dot(const std::string &name) {
    return (!name.empty())&&(name[0]=='.')&&
           (  (name.size()==1)||( (name[1]=='.')&&(name.size()==2) )  );
  }
  inline bool is_abspath(const std::string &name) {
    return (!name.empty())&&(name[0]=='/');
  }

#if (defined(_LARGEFILE64_SOURCE) && !defined(__x86_64__) && !defined(__ppc64__)) || defined(_WIN32)
// #if (defined _FILE_OFFSET_BITS && _FILE_OFFSET_BITS == 64) || defined _WIN32  // TODO?!
  std::string humanreadable_size(off64_t size);
#endif
  std::string humanreadable_size(off_t size);

  struct dstat_t {
    long long sum_space;
    long long free_space;
    // used_space=sum_space-free_space;
    bool readonly;
  };
  dstat_t get_diskstat(const std::string &path,bool rootspace=false);

};

#endif
