
#if !defined INCLUDED_CONFIGS_ERROR_ASSERT_OUT_IMPL_H
#define INCLUDED_CONFIGS_ERROR_ASSERT_OUT_IMPL_H

#include "vt/configs/error/common.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/assert_out.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/error/pretty_print_message.h"
#include "vt/context/context.h"

#include <tuple>
#include <type_traits>
#include <string>
#include <cassert>

#include <fmt/format.h>

namespace vt { namespace debug { namespace assert {

template <typename>
inline void assertOutExpr(
  bool fail, std::string const cond, std::string const& file, int const line,
  std::string const& func, ErrorCodeType error
) {
  auto msg = "Assertion failed:";
  auto reason = "";
  auto assert_fail_str = stringizeMessage(msg,reason,cond,file,line,func,error);
  ::vt::output(assert_fail_str,error,true,true,true);
  auto const& stack = ::vt::debug::stack::dumpStack();
  auto const& list_str = std::get<1>(stack);

  if (fail) {
    assert(false);
    std::exit(1);
  }
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, Args... args
) {
  auto msg = "Assertion failed:";
  auto assert_fail_str = stringizeMessage(msg,str,cond,file,line,func,error);
  ::vt::output(assert_fail_str,error,true,true,true);
  if (fail) {
    assert(false);
    std::exit(1);
  }
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, Args... args
) {
  auto const arg_str = ::fmt::format(str,args...);
  assertOut(false,cond,arg_str,file,line,func,error);
  if (fail) {
    assert(false);
    std::exit(1);
  }
}

}}} /* end namespace vt::debug::assert */

#endif /*INCLUDED_CONFIGS_ERROR_ASSERT_OUT_IMPL_H*/
