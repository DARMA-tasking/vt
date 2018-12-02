
#include "vt/config.h"
#include "vt/configs/arguments/args.h"

#include <string>
#include <vector>

#include <CLI/CLI.hpp>

namespace vt { namespace arguments {

/*static*/ CLI::App    ArgConfig::app{"vt"};
/*static*/ bool        ArgConfig::vt_color           = true;
/*static*/ bool        ArgConfig::vt_no_color        = false;
/*static*/ bool        ArgConfig::vt_auto_color      = false;
/*static*/ bool        ArgConfig::vt_no_sigint       = false;
/*static*/ bool        ArgConfig::vt_no_sigsegv      = false;
/*static*/ bool        ArgConfig::vt_no_terminate    = false;
/*static*/ bool        ArgConfig::vt_no_warn_stack   = false;
/*static*/ bool        ArgConfig::vt_no_assert_stack = false;
/*static*/ bool        ArgConfig::vt_no_abort_stack  = false;
/*static*/ bool        ArgConfig::vt_no_stack        = false;
/*static*/ std::string ArgConfig::vt_stack_file      = "";
/*static*/ std::string ArgConfig::vt_stack_dir       = "";
/*static*/ int32_t     ArgConfig::vt_stack_mod       = 0;
/*static*/ bool        ArgConfig::parsed             = false;

/*static*/ int ArgConfig::parse(int& argc, char**& argv) {
  if (parsed || argc == 0 || argv == nullptr) {
    return 0;
  }

  std::vector<std::string> args;
  for (auto i = 0; i < argc; i++) {
    args.push_back(std::string(argv[i]));
  }

  // fmt::print("argc={}, argv={}\n", argc, print_ptr(argv));

  /*
   * Flags for controlling the colorization of output from vt
   */
  auto always = "Always colorize output";
  auto never  = "Never colorize output";
  auto maybe  = "Use isatty to determine colorization of output";
  auto a = app.add_flag("-c,--vt_color",      vt_color,      always);
  auto b = app.add_flag("-n,--vt_no_color",   vt_no_color,   never);
  auto c = app.add_flag("-a,--vt_auto_color", vt_auto_color, maybe);
  auto colorGroup = "Print Coloring";
  a->group(colorGroup);
  b->group(colorGroup);
  c->group(colorGroup);
  b->excludes(a);
  b->excludes(c);

  /*
   * Flags for controlling the signals that VT tries to catch
   */
  auto no_sigint      = "Do not register signal handler for SIGINT";
  auto no_sigsegv     = "Do not register signal handler for SIGSEGV";
  auto no_terminate   = "Do not register handler for std::terminate";
  auto d = app.add_flag("--vt_no_SIGINT",    vt_no_sigint,    no_sigint);
  auto e = app.add_flag("--vt_no_SIGSEGV",   vt_no_sigsegv,   no_sigsegv);
  auto f = app.add_flag("--vt_no_terminate", vt_no_terminate, no_terminate);
  auto signalGroup = "Signa Handling";
  d->group(signalGroup);
  e->group(signalGroup);
  f->group(signalGroup);


  /*
   * Flags to control stack dumping
   */
  auto stack  = "Do not dump stack traces";
  auto warn   = "Do not dump stack traces when vtWarn(..) is invoked";
  auto assert = "Do not dump stack traces when vtAssert(..) is invoked";
  auto abort  = "Do not dump stack traces when vtAabort(..) is invoked";
  auto file   = "Dump stack traces to file instead of stdout";
  auto name   = "Name of file to dump stack backtrace";
  auto dir    = "Name of directory to write stack files";
  auto mod    = "Write stack dump if (node % vt_stack_mod) == 1 (default all)";
  auto g = app.add_flag("--vt_no_warn_stack",   vt_no_warn_stack,   warn);
  auto h = app.add_flag("--vt_no_assert_stack", vt_no_assert_stack, assert);
  auto i = app.add_flag("--vt_no_abort_stack",  vt_no_abort_stack,  abort);
  auto j = app.add_flag("--vt_no_stack",        vt_no_stack,        stack);
  auto k = app.add_option("--vt_stack_file",    vt_stack_file,      file, "");
  auto l = app.add_option("--vt_stack_dir",     vt_stack_dir,       dir,  "");
  auto m = app.add_option("--vt_stack_mod",     vt_stack_mod,       mod,  1);
  auto stackGroup = "Dump Stack Backtrace";
  g->group(stackGroup);
  h->group(stackGroup);
  i->group(stackGroup);
  j->group(stackGroup);
  k->group(stackGroup);
  l->group(stackGroup);
  m->group(stackGroup);

  /*
   * Flags for enabling load balancing and configuring it
   */


  /*
   * Run the parser!
   */
  app.allow_extras(true);
  try {
    app.parse(args);
  } catch (CLI::Error &e) {
    return app.exit(e);
  }

  /*
   * Put the arguments back into argc, argv, but properly order them based on
   * the input order by comparing between the current args
   */
  std::vector<std::string> ret_args;
  std::vector<std::size_t> ret_idx;

  // Reverse iterate (CLI11 reverses the order when they modify the args)
  for (auto iter = args.rbegin(); iter != args.rend(); ++iter) {
    for (auto i = 0; i < argc; i++) {
      if (std::string(argv[i]) == *iter) {
        ret_idx.push_back(i);
      }
    }
    ret_args.push_back(*iter);
  }

  // Use the saved index to setup the new_argv and new_argc
  int new_argc = ret_args.size();
  char** new_argv = new char*[new_argc];
  for (auto i = 0; i < new_argc; i++) {
    new_argv[i] = argv[ret_idx[i]];
  }

  // Set them back with all vt arguments elided
  argc = new_argc;
  argv = new_argv;

  parsed = true;
  return 1;
}

}} /* end namespace vt::arguments */
