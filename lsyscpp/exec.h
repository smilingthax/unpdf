#ifndef _EXEC_H
#define _EXEC_H

#include <vector>
#include <string>

class Sys {
public:
  static int execute(const char *execpath,...); // ..., NULL)
  static int execute(const std::string &execpath,const std::vector<std::string> &args);
protected:
  static int do_exec(const char *execpath,const char **args);
};

#endif
