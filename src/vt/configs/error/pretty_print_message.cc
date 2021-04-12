#include "vt/configs/error/pretty_print_message.h"

#include "vt/configs/error/common.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/generated/vt_git_revision.h"
#include "vt/context/context.h"

#include <string>
#include <unistd.h> // gethostname

#include "fmt/core.h"

namespace vt { namespace debug {

static std::string s_hostname = std::string();

inline std::string& getCachedHostname() {
  if (s_hostname.empty()) {
    char temp[1024] = {0};
    gethostname(temp, 1024);
    s_hostname = std::string(temp);
  }
  return s_hostname;
}

std::string stringizeMessage(
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

  std::string dirty = "";
  if (strncmp(vt_git_clean_status.c_str(), "DIRTY", 5) == 0) {
    dirty = red + std::string("*dirty*") + reset;
  } else {
    dirty = green + std::string("clean") + reset;
  }

  std::string const& hostname = getCachedHostname();

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

