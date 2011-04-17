#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <string>
#include <stdexcept>

class FS_except : public std::exception {
public:
  FS_except(int errnum) throw();
  ~FS_except() throw() {}
  const char *what() const throw() { return errtext.c_str(); }
  int err_no() const throw() { return errnum; }
private:
  std::string errtext;
  int errnum;
};


namespace FS {
  std::string cwd();
  std::string abspath(const std::string &filename);
  std::string dirname(const std::string &filename);
  std::string basename(const std::string &filename);
  bool exists(const std::string &filename);
  bool is_file(const std::string &path);
  bool is_dir(const std::string &path);
  void create_dir(const std::string &dirname); 
  std::string joinPath(const std::string &a1,const std::string &a2);
  std::pair<std::string,std::string> extension(const std::string &filename);

  inline bool is_special_dot(const std::string &name) {
    return (!name.empty())&&(name[0]=='.')&&
           (  (name.size()==1)||( (name[1]=='.')&&(name.size()==2) )  );
  }
  inline bool is_abspath(const std::string &name) {
    return (!name.empty())&&(name[0]=='/');
  }

#ifdef _LARGEFILE64_SOURCE
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