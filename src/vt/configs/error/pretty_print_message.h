
#if !defined INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_MESSAGE_H
#define INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_MESSAGE_H

#include "vt/configs/error/common.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/generated/vt_git_revision.h"
#include "vt/context/context.h"

#include <string>
#include <unistd.h>

#include <fmt/format.h>

namespace vt { namespace debug {

inline std::string stringizeMessage(
  std::string const& msg, std::string const& reason, std::string const& cond,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error
) {
  auto node            = ::vt::debug::preNode();
  auto nodes           = ::vt::debug::preNodes();
  auto vt_pre          = ::vt::debug::vtPre();
  auto node_str        = ::vt::debug::proc(node);
  auto prefix          = vt_pre + node_str + " ";
  auto green           = ::vt::debug::green();
  auto blue            = ::vt::debug::blue();
  auto reset           = ::vt::debug::reset();
  auto bred            = ::vt::debug::bred();
  auto red             = ::vt::debug::bred();
  auto magenta         = ::vt::debug::magenta();
  auto assert_reason   = reason != "" ? ::fmt::format(
    "{}{:>20} {}{}{}\n", prefix, "Reason:", magenta, reason, reset
  ) : "";
  auto message         = ::fmt::format(
    "{}{:>20} {}{}{}\n", prefix, "Type:", red, msg, reset
  );
  auto assert_cond     = cond != "" ? ::fmt::format(
    "{}{:>20} {}({}){}\n", prefix, msg, bred, cond, reset
  ) : message;

  char hostname[1024];
  gethostname(hostname, 1024);

  std::string dirty = "";
  if (strncmp(vt_git_clean_status.c_str(), "DIRTY", 5) == 0) {
    dirty = red + std::string("*dirty*") + reset;
  } else {
    dirty = green + std::string("clean") + reset;
  }

  auto assert_fail_str = ::fmt::format(
    "{}\n"
    "{}"
    "{}"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}{}{}\n"
    "{}{:>20} {}\n"
    "{}{:>20} {}{}{}\n"
    "{}\n",
    prefix,
    assert_reason,
    assert_cond,
    prefix, "Node:",        blue,    node,              reset,
    prefix, "Num Nodes:",   blue,    nodes,             reset,
    prefix, "File:",        green,   file,              reset,
    prefix, "Line:",        green,   line,              reset,
    prefix, "Function:",    green,   func,              reset,
    prefix, "Code:",        green,   error,             reset,
    prefix, "Build SHA:",   magenta, vt_git_sha1,       reset,
    prefix, "Build Ref:",   magenta, vt_git_refspec,    reset,
    prefix, "Description:", magenta, vt_git_description,reset,
    prefix, "GIT Repo:",    dirty,
    prefix, "Hostname:",    magenta, hostname,          reset,
    prefix
  );
  return assert_fail_str;
}

}} /* end namespace vt::debug */

#endif /*INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_MESSAGE_H*/
