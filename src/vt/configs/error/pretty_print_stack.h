
#if !defined INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_STACK_H
#define INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_STACK_H

#include "vt/config.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/debug/debug_colorize.h"

#include <vector>
#include <tuple>
#include <string>

namespace vt { namespace debug { namespace stack {

inline std::string prettyPrintStack(StackVectorType const& stack) {
  auto green      = ::vt::debug::green();
  auto red        = ::vt::debug::red();
  auto bred       = ::vt::debug::bred();
  auto reset      = ::vt::debug::reset();
  auto bd_green   = ::vt::debug::bd_green();
  auto magenta    = ::vt::debug::magenta();
  auto blue       = ::vt::debug::blue();
  auto yellow     = ::vt::debug::yellow();
  auto vt_pre     = ::vt::debug::vtPre();
  auto node       = ::vt::theContext()->getNode();
  auto node_str   = ::vt::debug::proc(node);
  auto prefix     = vt_pre + node_str + " ";
  auto seperator  = fmt::format("{}{}{:-^120}{}\n", prefix, yellow, "", reset);
  auto title_node = fmt::format("on Node {}", node);
  auto title      = fmt::format(" Dump Stack Backtrace {} ", title_node);

  std::string out = "";

  out += fmt::format("{}", seperator);
  out += fmt::format("{}{}{:-^120}{}\n", prefix, yellow, title, reset);
  out += fmt::format("{}", seperator);

  int i = 0;
  for (auto&& t : stack) {
    auto ret_str = fmt::format(
      "{}{}{:<3}{} {}{:<3} {:<13}{} {}{}{} + {}{}\n",
      prefix,
      bred, i, reset,
      magenta, std::get<0>(t), std::get<1>(t), reset,
      green,   std::get<2>(t), reset,
      std::get<3>(t), reset
    );
    out += ret_str;
    i++;
  }

  //out += seperator + seperator + seperator;

  return out;
}

}}} /* end namespace vt::debug::stack */

#endif /*INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_STACK_H*/
