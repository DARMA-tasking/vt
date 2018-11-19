
#if !defined INCLUDED_CONFIGS_ERROR_ASSERT_OUT_IMPL_H
#define INCLUDED_CONFIGS_ERROR_ASSERT_OUT_IMPL_H

#include "vt/configs/error/common.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/assert_out.h"

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
  auto const assert_fail_str = ::fmt::format(
    "Assertion failed: ({}) : File: {}\nLine: {}\nFunction: {}\n",
    cond,file,line,func
  );
  ::vt::output(assert_fail_str,error,false,true);
  if (fail) {
    assert(false);
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
  auto const assert_fail_str = ::fmt::format(
    "Assertion failed: ({}) : {}\nFile: {}\nLine: {}\nFunction: {}\n",
    cond,str,file,line,func
  );
  ::vt::output(assert_fail_str,error,false,true);
  if (fail) {
    assert(false);
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
  }
}

}}} /* end namespace vt::debug::assert */

#endif /*INCLUDED_CONFIGS_ERROR_ASSERT_OUT_IMPL_H*/
