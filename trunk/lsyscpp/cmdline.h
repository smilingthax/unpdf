#ifndef _CMDLINE_H
#define _CMDLINE_H

#include <vector>
#include <string>

/**
  This is a Commandline parser base class.
  It is used by deriving your specialized class from it.

  The usual procedure is to make the variables for use in your programm public.
  In the constructor add/add_param the variables with the respective options
  and usage-text. You may also want to override the parse() function
  to first call the base-class's parse() and then do validity checking of the parameters.

  If a parameter is not set, the values are:
    bool: false
    int: INT_MIN (usually -2147483647-1)
    count: 0
    string,vec<int>,vec<string>: empty
  If you want to distinguish an empty string from "not given" use
  vec<string> and check its size.

 */
class Cmdline {
public:
  Cmdline(bool with_opt_opt=false,bool with_usage=true,const char *cmdlopts="[options]\n")
         : do_optopt(with_opt_opt),usage_cmdlopts(cmdlopts?cmdlopts:"\n") {
    set_usage(NULL,NULL);
    if (with_usage) { // if you don't want it first -> set >with_usage=false and do the following yourself.
      add("h","help",do_usage);
    }
  }
  virtual ~Cmdline() {}
  virtual bool parse(int argc,char const *const *argv); // false when parse failed or --help given
  bool parse_fromfile(const std::string &argsfile,const char *pname=NULL);
protected:
  void add_param(const char *shortopt,const char *longopt,int &resvar,const char *usagestr=NULL);
  void add_param(const char *shortopt,const char *longopt,std::string &resvar,const char *usagestr=NULL);
  void add_param(const char *shortopt,const char *longopt,std::vector<int> &resvar,const char *usagestr=NULL);
  void add_param(const char *shortopt,const char *longopt,std::vector<std::string> &resvar,const char *usagestr=NULL);
  void add(const char *shortopt,const char *longopt,bool &resvar,const char *usagestr=NULL);
  void add(const char *shortopt,const char *longopt,int &resvar,const char *usagestr=NULL); // counts the occurences
  void add_usage(const char *usagestr=NULL);
  void set_usage(const char *pretext,const char *posttext);

  bool is_default(const char *opt) const; // >opt "--arg" or "-a" to distinguish long vs short opt
  void set(const char *opt,const char *addstr=NULL); // convenience for do_parse; >addstr maybe parameter; only to be used in parse()
  void do_clear(const char *progname=NULL);
  bool do_parse(int argc,char const *const *argv);
  std::vector<std::string> get_fromfile(const std::string &argsfile) const;

  std::string usage_opts() const;
  void usage(const char *pname=NULL) const;
  std::vector<std::string> opt_opts;
  bool do_usage;
private:
  bool do_optopt;
  struct cmdopt {
    enum type_t { TYPE_USAGE, TYPE_BOOL, TYPE_COUNT, TYPE_INT, TYPE_STRING, TYPE_MULTIINT, TYPE_MULTISTRING };

    cmdopt(const char *_shortopt,const char *_longopt,type_t _type,
           bool *_bval,int *_ival,std::string *_strval,std::vector<int> *_ivec,std::vector<std::string> *_strvec,const char *_usagestr);
    void clear();
    bool do_opt();
    bool parse_opt(const char *arg,int pos);
    bool is_default() const;

    std::string shortopt,longopt;
    type_t type;
    union {
      bool *bval;
      int *ival;
      std::string *strval;
      std::vector<int> *ivec;
      std::vector<std::string> *strvec;
    } resvar;
    std::string usagestr;
  };
  std::vector<cmdopt> cmds;
  std::string progname; // filled after parse()
  std::string usage_pre,usage_post;
  std::string usage_cmdlopts; // text after programmname; default "[options]". Interesting esp. for opt_opts

  void check_param(const char *shortopt,const char *longopt) const;
  template <std::string Cmdline::cmdopt::*lsopt>
  bool parse_one(int argc,char const *const *argv,int &cur_arg,int &pos);
};

#endif
