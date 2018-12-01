
#include "vt/config.h"
#include "vt/configs/arguments/args.h"

#include <CLI/CLI.hpp>

namespace vt { namespace arguments {

/*static*/ CLI::App ArgConfig::app{"vt"};
/*static*/ bool     ArgConfig::vt_color        = true;
/*static*/ bool     ArgConfig::vt_no_color     = false;
/*static*/ bool     ArgConfig::vt_auto_color   = false;
/*static*/ bool     ArgConfig::vt_no_sigint    = false;
/*static*/ bool     ArgConfig::vt_no_sigsegv   = false;
/*static*/ bool     ArgConfig::vt_no_terminate = false;
/*static*/ bool     ArgConfig::vt_warn_stack   = true;
/*static*/ bool     ArgConfig::vt_assert_stack = true;
/*static*/ bool     ArgConfig::vt_abort_stack  = true;
/*static*/ bool     ArgConfig::vt_stack        = true;
/*static*/ bool     ArgConfig::parsed          = false;

/*static*/ int ArgConfig::parse(int argc, char** argv) {
  if (parsed) {
    return 0;
  }

  /*
   * Flags for controlling the colorization of output from vt
   */
  auto always = "Always colorize output";
  auto never  = "Never colorize output";
  auto maybe  = "Use isatty to determine colorization of output";
  auto a = app.add_flag("-c,--vt_color",      vt_color,      always, true);
  auto b = app.add_flag("-n,--vt_no_color",   vt_no_color,   never, false);
  auto c = app.add_flag("-a,--vt_auto_color", vt_auto_color, maybe, false);
  b->excludes(a);
  b->excludes(c);

  /*
   * Flags for controlling the signals that VT tries to catch
   */
  auto no_sigint      = "Do not register handler for SIGINT";
  auto no_sigsegv     = "Do not register handler for SIGSEGV";
  auto no_terminate   = "Do not register handler for std::terminate";
  auto d = app.add_flag("--vt_no_SIGINT",    vt_no_sigint,    no_sigint);
  auto e = app.add_flag("--vt_no_SIGSEGV",   vt_no_sigsegv,   no_sigsegv);
  auto f = app.add_flag("--vt_no_terminate", vt_no_terminate, no_terminate);


  /*
   * Flags to control stack dumping
   */
  auto stack  = "Dump stack traces";
  auto warn   = "Dump stack traces when vtWarn(..) is invoked";
  auto assert = "Dump stack traces when vtAssert(..) is invoked";
  auto abort  = "Dump stack traces when vtAabort(..) is invoked";
  auto g = app.add_flag("--vt_warn_stack",   vt_warn_stack,   warn,   true);
  auto h = app.add_flag("--vt_assert_stack", vt_assert_stack, assert, true);
  auto i = app.add_flag("--vt_abort_stack",  vt_abort_stack,  abort,  true);
  auto j = app.add_flag("--vt_stack",        vt_stack,        stack,  true);

  /*
   * Flags for enabling load balancing and configuring it
   */


  /*
   * Run the parser!
   */
  try {
    app.parse(argc, argv);
  } catch (CLI::Error &e) {
    return app.exit(e);
  }

  parsed = true;
  return 1;
}

}} /* end namespace vt::arguments */
