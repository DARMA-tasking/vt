
#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H

#include "vt/config.h"

#include <CLI/CLI.hpp>

namespace vt { namespace arguments {

struct ArgConfig {

  static int parse(int& argc, char**& argv);

public:
  static bool vt_color;
  static bool vt_no_color;
  static bool vt_auto_color;

  static bool vt_no_sigint;
  static bool vt_no_sigsegv;
  static bool vt_no_terminate;

  static bool vt_no_warn_stack;
  static bool vt_no_assert_stack;
  static bool vt_no_abort_stack;
  static bool vt_no_stack;
  static std::string vt_stack_file;
  static std::string vt_stack_dir;

private:
  static CLI::App app;
  static bool parsed;
};

}} /* end namespace vt::arguments */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
