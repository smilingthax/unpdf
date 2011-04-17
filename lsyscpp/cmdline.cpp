#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "cmdline.h"
#include "exception.h"
#include <errno.h>

using namespace std;

// {{{ Cmdline::cmdopt
Cmdline::cmdopt::cmdopt(const char *_shortopt,const char *_longopt,type_t _type,bool *_bval,int *_ival,string *_strval,vector<int> *_ivec,vector<string> *_strvec,const char *_usagestr)
    : shortopt(_shortopt?_shortopt:""),longopt(_longopt?_longopt:""),type(_type),usagestr(_usagestr?_usagestr:"") 
{
  switch (type) {
  case TYPE_USAGE:
    break;
  case TYPE_BOOL:
    assert(_bval);
    resvar.bval=_bval;
    break;
  case TYPE_INT:
  case TYPE_COUNT:
    assert(_ival);
    resvar.ival=_ival;
    break;
  case TYPE_STRING:
    assert(_strval);
    resvar.strval=_strval;
    break;
  case TYPE_MULTIINT:
    assert(_ivec);
    resvar.ivec=_ivec;
    break;
  case TYPE_MULTISTRING:
    assert(_strvec);
    resvar.strvec=_strvec;
    break;
  }
}

void Cmdline::cmdopt::clear() 
{
  switch (type) {
  case TYPE_USAGE:
    break;
  case TYPE_BOOL:
    *resvar.bval=false;
    break;
  case TYPE_INT:
    *resvar.ival=INT_MIN;
    break;
  case TYPE_COUNT:
    *resvar.ival=0;
    break;
  case TYPE_STRING:
    resvar.strval->clear();
    break;
  case TYPE_MULTIINT:
    resvar.ivec->clear();
    break;
  case TYPE_MULTISTRING:
    resvar.strvec->clear();
    break;
  }
}

bool Cmdline::cmdopt::is_default() const
{
  switch (type) {
  case TYPE_USAGE:
    return true; // some sensible value
  case TYPE_BOOL:
    return (*resvar.bval==false);
  case TYPE_INT:
    return (*resvar.ival==INT_MIN);
  case TYPE_COUNT:
    return (*resvar.ival==0);
  case TYPE_STRING:
    return resvar.strval->empty();
  case TYPE_MULTIINT:
    return resvar.ivec->empty();
  case TYPE_MULTISTRING:
    return resvar.strvec->empty();
  default:
    assert(0);
    return true;
  }
}

bool Cmdline::cmdopt::do_opt()
{
  switch (type) {
  case TYPE_USAGE:
    assert(0);
    return true;
  case TYPE_BOOL:
    *resvar.bval=true;
    return true;
  case TYPE_COUNT:
    ++*resvar.ival;
    return true;
  default:
    return false;
  }
}

bool Cmdline::cmdopt::parse_opt(const char *arg,int pos)
{
  if (!arg[pos]) {
    return false;
  }
  if ( (type==TYPE_INT)||(type==TYPE_MULTIINT) ) {
    char *tmp;
    int res=strtol(arg+pos,&tmp,10);
    if (*tmp) {
      return false;
    }
    if (type==TYPE_INT) {
      *resvar.ival=res;
    } else {
      resvar.ivec->push_back(res);
    }
  } else if (type==TYPE_STRING) {
    resvar.strval->assign(arg+pos);
  } else if (type==TYPE_MULTISTRING) {
    resvar.strvec->push_back(string(arg+pos));
  } else {
    assert(false); // do_opt()==true ??
    return false;
  }
  return true;
}
// }}}


void Cmdline::check_param(const char *shortopt,const char *longopt) const
{
  if ( (!shortopt)&&(!longopt) ) {
    assert(false);
    throw FS_except(EINVAL);
  }
  // check for collisions
  for (int iA=0;iA<(int)cmds.size();iA++) {
    if ( (shortopt)&&(strcmp(cmds[iA].shortopt.c_str(),shortopt)==0) ) {
      throw FS_except(EEXIST);
    } else if ( (longopt)&&(strcmp(cmds[iA].longopt.c_str(),longopt)==0) ) {
      throw FS_except(EEXIST);
    }
  }
}

void Cmdline::add_param(const char *shortopt,const char *longopt,int &resvar,const char *usagestr)
{
  check_param(shortopt,longopt);
  cmds.push_back(cmdopt(shortopt,longopt,cmdopt::TYPE_INT,NULL,&resvar,NULL,NULL,NULL,usagestr));
}

void Cmdline::add_param(const char *shortopt,const char *longopt,string &resvar,const char *usagestr)
{
  check_param(shortopt,longopt);
  cmds.push_back(cmdopt(shortopt,longopt,cmdopt::TYPE_STRING,NULL,NULL,&resvar,NULL,NULL,usagestr));
}

void Cmdline::add_param(const char *shortopt,const char *longopt,vector<int> &resvar,const char *usagestr)
{
  check_param(shortopt,longopt);
  cmds.push_back(cmdopt(shortopt,longopt,cmdopt::TYPE_MULTIINT,NULL,NULL,NULL,&resvar,NULL,usagestr));
}

void Cmdline::add_param(const char *shortopt,const char *longopt,vector<string> &resvar,const char *usagestr)
{
  check_param(shortopt,longopt);
  cmds.push_back(cmdopt(shortopt,longopt,cmdopt::TYPE_MULTISTRING,NULL,NULL,NULL,NULL,&resvar,usagestr));
}

void Cmdline::add(const char *shortopt,const char *longopt,bool &resvar,const char *usagestr)
{
  check_param(shortopt,longopt);
  cmds.push_back(cmdopt(shortopt,longopt,cmdopt::TYPE_BOOL,&resvar,NULL,NULL,NULL,NULL,usagestr));
}

void Cmdline::add(const char *shortopt,const char *longopt,int &resvar,const char *usagestr)
{
  check_param(shortopt,longopt);
  cmds.push_back(cmdopt(shortopt,longopt,cmdopt::TYPE_COUNT,NULL,&resvar,NULL,NULL,NULL,usagestr));
}

void Cmdline::add_usage(const char *usagestr)
{
  cmds.push_back(cmdopt(NULL,NULL,cmdopt::TYPE_USAGE,NULL,NULL,NULL,NULL,NULL,usagestr));
}

template <string Cmdline::cmdopt::*lsopt>
bool Cmdline::parse_one(int argc,char const *const *argv,int &cur_arg,int &pos)
{
//  const char *prefix=(lsopt==&cmdopt::longopt)?"--":"-";
  int iA;
  for (iA=0;iA<(int)cmds.size();iA++) {
    if (cmds[iA].type==cmdopt::TYPE_USAGE) {
      continue;
    }
    const int optlen=(cmds[iA].*lsopt).size();
    if ( (optlen)&&(strncmp(argv[cur_arg]+pos,(cmds[iA].*lsopt).c_str(),optlen)==0) ) {
      pos+=optlen;
      if (!cmds[iA].do_opt()) { // needs arg
        if (!argv[cur_arg][pos]) {
          if (++cur_arg>=argc) { // not enough
            // ERROR(Option 'prefix cmds[iA].*lsopt' requires an argument)
            return false;
          }
          pos=0;
        } else if (lsopt==&cmdopt::longopt) {
          if (argv[cur_arg][pos]!='=') {
            // ERROR(Unknown option 'prefix argv[cur_arg][pos]')
            // TODO? pos-=optlen; continue;
            return false;
          }
          pos++;
        }
        if (!cmds[iA].parse_opt(argv[cur_arg],pos)) {
          // ERROR(Invalid argument for option 'prefix cmds[iA].*lsopt')
          return false;
        }
        pos=strlen(argv[cur_arg]);
      }
      break;
    }
  }
  if (iA==(int)cmds.size()) { // not found
    // ERROR(Unknown option 'prefix argv[cur_arg][pos]')
    // TODO? as opt_opt?
    return false;
  }
  return true;
}

bool Cmdline::parse(int argc,char const *const *argv)
{
  assert( (argc>0)&&(argv) );
  assert(argv[0]);

  do_clear(argv[0]);
  return do_parse(argc,argv);
}

bool Cmdline::is_default(const char *opt) const
{
  if ( (!opt)||(opt[0]!='-') ) {
    assert(false);
    throw FS_except(EINVAL);
  }
  if (opt[1]=='-') { // long
    for (int iA=0;iA<(int)cmds.size();iA++) {
      if ( (strcmp(cmds[iA].longopt.c_str(),opt+2)==0) ) {
        return cmds[iA].is_default();
      }
    }
  } else { // short
    for (int iA=0;iA<(int)cmds.size();iA++) {
      if ( (strcmp(cmds[iA].shortopt.c_str(),opt+1)==0) ) {
        return cmds[iA].is_default();
      }
    }
  }
  // ERROR(Unknown option)
  throw FS_except(EINVAL);
}

void Cmdline::set(const char *opt,const char *addstr)
{
  char const *const argv[3]={NULL,opt,addstr};
  if (!do_parse(((addstr)?2:1)+1,argv)) {
    // ERROR(Bad option specification)
    throw FS_except(EINVAL);
  }
}

void Cmdline::do_clear(const char *pname)
{
  if (pname) {
    progname.assign(pname);
  }
  // first set all options to default
  for (int iA=0;iA<(int)cmds.size();iA++) {
    cmds[iA].clear();
  }
}

bool Cmdline::do_parse(int argc,char const *const *argv)
{
  // parse
  /*
      -abc text
      -abctext
      -a -b -c text
      --a-long --b-long --c-long=text
      --a-long --b-long --c-long text
  */
  for (int iB=1;iB<argc;iB++) {
    int pos=0;
    if (argv[iB][pos]=='-') {
      pos++;
      if (argv[iB][pos]=='-') { // one longopt
        pos++;
        bool res=parse_one<&cmdopt::longopt>(argc,argv,iB,pos);
        if (!res) {
          return res;
        }
        if (argv[iB][pos]) {
          // ERROR(Argument given for option '--cmds[iA].longopt')
          return false;
        }
      } else { // one or more shortopts, first match
        do {
          bool res=parse_one<&cmdopt::shortopt>(argc,argv,iB,pos);
          if (!res) {
            return res;
          }
        } while (argv[iB][pos]);
      }
    } else { // optional args
      if (!do_optopt) {
        // ERROR(Unknown option 'argv[iB]')
        return false;
      }
      opt_opts.push_back(string(argv[iB]));
    }
  }
  return !do_usage;
}

bool Cmdline::parse_fromfile(const std::string &argsfile,const char *pname)
{
  vector<string> args=get_fromfile(argsfile);
  if ( (args.empty())&&(errno!=0) ) {
    // TODO? quoting error: errno==EBADMSG
    return false;
  }
  
  vector<const char *> argv;
  argv.resize(args.size()+1,NULL);
  argv[0]=pname;
  for (int iA=0;iA<(int)args.size();iA++) {
    argv[iA+1]=args[iA].c_str();
  }
  // TODO TODO TODO:
  // When >pname is set, assume the caller wants to treat the arguments from
  // >argsfile _alone_ as the commandline.
  // When it is not set, we assume that parse(), and thus do_clear(), has already been
  // called, so we call do_parse() directly, avioding do_clear() and also keeping a already
  // extracted >progname.
  if (pname) {
    return parse(argv.size(),&argv[0]);
  } else {
    return do_parse(argv.size(),&argv[0]);
  }
}

std::vector<string> Cmdline::get_fromfile(const std::string &argsfile) const
{
  vector<string> args;
  
  FILE *f;
  if ((f=fopen(argsfile.c_str(),"r"))==NULL) {
    args.clear();
    assert(errno!=0);
    return args;
  }
#define BUFSIZE 1024
  char buf[BUFSIZE];
  
  enum { NONE,DQUOTE,SQUOTE } qmode=NONE;
  string curarg;
  bool active=false;
  while (!feof(f)) {
    if (!fgets(buf,BUFSIZE,f)) {
      break;
    }
    int iA=0;
    if (qmode==NONE) {
      iA=strspn(buf," \n\t"); 
      if (buf[iA]=='#') { // comment
        continue;
      }
    }
    for (;buf[iA];iA++) {
      if (qmode==NONE) {
        if (buf[iA]=='"') {
          qmode=DQUOTE;
          active=true;
        } else if (buf[iA]=='\'') {
          qmode=SQUOTE;
          active=true;
        } else {
          int cnt=strspn(buf+iA," \n\t");
          iA+=cnt;
          if (cnt) {
            // new arg
            args.push_back(curarg);
            curarg.clear();
            active=false;
            iA--;
          } else {
            curarg.push_back(buf[iA]);
            active=true;
          }
        }
      } else if (qmode==DQUOTE) {
        if (buf[iA]=='"') {
          qmode=NONE;
        } else {
          curarg.push_back(buf[iA]); // TODO: perhaps honour \n, ...
        }
      } else if (qmode==SQUOTE) {
        if (buf[iA]=='\'') {
          qmode=NONE;
        } else {
          curarg.push_back(buf[iA]);
        }
      }
    }
  }
  fclose(f);
  if (qmode!=NONE) {
    args.clear();
    errno=EBADMSG; // quoting error
    return args;
  }
  if (active) {
    args.push_back(curarg);
  }
  errno=0;
  return args;
}

string Cmdline::usage_opts() const
{
  string ret;
  for (int iA=0;iA<(int)cmds.size();iA++) {
    if (cmds[iA].type==cmdopt::TYPE_USAGE) {
      ret.append(cmds[iA].usagestr);
      ret.push_back('\n');
      continue;
    } else if (cmds[iA].shortopt.empty()) {
      ret.append("  --");
      ret.append(cmds[iA].longopt);
    } else if (cmds[iA].longopt.empty()) {
      ret.append("  -");
      ret.append(cmds[iA].shortopt);
    } else {
      ret.append("  -");
      ret.append(cmds[iA].shortopt);
      ret.append(" or --");
      ret.append(cmds[iA].longopt);
    }
    if (cmds[iA].type==cmdopt::TYPE_INT) {
      ret.append(" [int]");
    } else if (cmds[iA].type==cmdopt::TYPE_MULTIINT) {
      ret.append(" [int] *");
    } else if (cmds[iA].type==cmdopt::TYPE_STRING) {
      ret.append(" [string]");
    } else if (cmds[iA].type==cmdopt::TYPE_MULTISTRING) {
      ret.append(" [string] *");
    }
    if (!cmds[iA].usagestr.empty()) {
      ret.append(": ");
      ret.append(cmds[iA].usagestr);
    }
    ret.push_back('\n');
  }
  return ret;
}

void Cmdline::set_usage(const char *pretext,const char *posttext)
{
  usage_pre.assign(pretext?pretext:"\n");
  usage_post.assign(posttext?posttext:"\n");
}

void Cmdline::usage(const char *pname) const
{
  printf("%s"
         "Usage: %s %s"
         "\n"
         "%s"
         "%s",
         usage_pre.c_str(),
         pname?pname:progname.c_str(),usage_cmdlopts.c_str(),
         usage_opts().c_str(),
         usage_post.c_str());
}
