#include "filesystem.h"
#include "exec.h"
#include "cmdline.h"
#include "fastmatch.h"
#include "dir_iter.h"
#include <stdio.h>

using namespace std;

class My_Cmdline : public Cmdline {
public:
  My_Cmdline() : Cmdline(false,true) {
    add("b","bool",a,"A boolean option (flag)");
    add("c","count",b,"A counting boolean option");
    add_param("i","int",c,"An option with integer argument");
    add_param("s","string",d,"An option with string argument");
  }
  bool parse(int argc,char const *const *argv)
  {
    if (!Cmdline::parse(argc,argv)) {
      usage();
      return false;
    }
    printf("Got options:\n"
           "bool: %d\n"
           "count: %d\n"
           "int: %d\n"
           "string: %s\n",a,b,c,d.c_str());
    return true;
  }

  bool a;
  int b,c;
  string d;
};

int main(int argc,char **argv)
{
  try {
    printf("Free: %s\n",FS::humanreadable_size(FS::get_diskstat("/").free_space).c_str());
#if 0
    My_Cmdline opt;
    bool res=opt.parse(argc,argv);
    printf("Cmdline::parse returned: %d\n",res);
#endif
#if 1
    printf("CWD: %s\n",FS::cwd().c_str());
    printf("BASECWD: %s\n",FS::basename(FS::cwd()).c_str());
    printf("DIRCWD: %s\n",FS::dirname(FS::cwd()).c_str());
    printf("ARG0: %s\n",argv[0]);
    printf("ABSARG0: %s\n",FS::abspath(argv[0]).c_str());
    printf("exists: ARG0 %d xMakefile %d\n",FS::exists(argv[0]),FS::exists("xMakefile"));
    printf("is_dir: x %d Makefile %d\n",FS::is_dir("x"),FS::is_dir("Makefile"));

    printf("LS:\n");
    for (dir_iterator it(".");!it.end();++it) {
      printf("  %s (%s)\n",it->c_str(),it.fullpath().c_str());
    }

    printf("EXEC LS:\n");
//    Sys::execute("ls",NULL);
    Sys::execute("/bin/ls",NULL);
#endif
  } catch (exception &ex) {
    printf("Ex: %s\n",ex.what());
  }
  
  return 0;
}
