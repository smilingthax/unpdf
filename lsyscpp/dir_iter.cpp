#include "dir_iter.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include "filesystem.h"

using namespace std;

// {{{ dir_iterator
struct dir_iterator::dir_iterator_impl {
  std::string path,current;
  DIR *d;
};

dir_iterator::dir_iterator(const std::string &path)
{
  impl=new dir_iterator_impl;

  impl->path=path;
  impl->d=opendir(path.c_str());
  if (!impl->d) {
    throw FS_except(errno,"opendir",path.c_str());
  }
  ++(*this);
}

dir_iterator::~dir_iterator()
{
  if (impl->d) {
    closedir(impl->d);
  }
  delete impl;
}

const dir_iterator &dir_iterator::operator++()
{
  struct dirent *de;
  // assert(impl->d);  // user is not allowed to call ++ on end()
  de=readdir(impl->d);
  if (!de) { // done
    closedir(impl->d);
    impl->d=NULL;
    impl->current.clear();
    return *this;
  }
  impl->current.assign(de->d_name);
  return *this;
}

const string &dir_iterator::operator*() const
{
  return impl->current;
}

const string *dir_iterator::operator->() const
{
  return &impl->current;
}

bool dir_iterator::end() const
{
  return (!impl->d);
}

string dir_iterator::fullpath() const
{
  return FS::joinPath(impl->path,impl->current);
}
// }}}
