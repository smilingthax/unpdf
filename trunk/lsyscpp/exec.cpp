#include "exec.h"
#include <stdarg.h>
#include <assert.h>
#include <sys/wait.h>
#include <errno.h>
#include "filesystem.h"

/*
int fds[2];
if (pipe(fds)==-1) {
  throw "cannot create pipe";
}
// Set close-on-exec on write end of pipe.
int flags = fcntl(fds[1], F_GETFD);
if (flags==-1) {
  throw "cannot get fcntl flags";
}
flags|=FD_CLOEXEC;
if (fcntl(fds[1],F_SETFD,flags)==-1) {
  throw "cannot set close-on-exec";
}

if (...child) {
  // Close all open file descriptors.
  int maxFd = static_cast<int>(sysconf(_SC_OPEN_MAX));
  for(int fd = 0; fd < maxFd; ++fd) {
    if(fd != fds[1]) // Don’t close write end of pipe.  {
      close(fd);
    }
  }

  execv(exe, argv);

  const char msg[] = "exec failed";
  write(fds[1], msg, sizeof(msg) - 1);
  _exit(1);
} else {
  while(read(fds[0], &c, 1) > 0) {
    ...
    err << c;
  }
}

*/


using namespace std;

#include <stdio.h>
#include <unistd.h>

int Sys::do_exec(const char *execpath,const char **args)
{
  int pid=fork();
  if (pid<0) {
//    throw FS_except(errno);
    return -1;
  } else if (pid==0) { // child
    execv(execpath,(char *const *)args);
printf("%s\n",execpath);
    printf("exec failed: %m\n");
    // ... write to a pipe the errno
    _exit(255);
    // do NOT throw, we are in a different thread!
  } else {
    int status;
    if (waitpid(pid,&status,0)==-1) {
//      throw FS_except(errno);
      return -1;
    }

    if (WIFEXITED(status)) {
      // if errno...
      return WEXITSTATUS(status);
    } else {
      return -1;
    }
  }
}

int Sys::execute(const char *execpath,...)
{
  va_list ap;
  vector<const char *> args;

  string arg0=FS::basename(string(execpath));
  args.push_back(arg0.c_str());

  va_start(ap,execpath);
  char *tmp;
  do {
    tmp=va_arg(ap,char *);
    args.push_back(tmp); // we also require trailing NULL in >args
  } while (tmp);
  va_end(ap);

  return do_exec(execpath,&args[0]);
}

int Sys::execute(const string &execpath,const vector<string> &args)
{
  vector<const char *> cargs;

  string arg0=FS::basename(execpath);
  cargs.push_back(arg0.c_str());

  for (int iA=0;iA<(int)args.size();iA++) {
    cargs.push_back(args[iA].c_str());
  }
  cargs.push_back(NULL);

  return do_exec(execpath.c_str(),&cargs[0]);
}

