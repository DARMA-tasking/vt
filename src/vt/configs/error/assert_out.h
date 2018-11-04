
#if !defined INCLUDED_CONFIGS_ERROR_ASSERT_OUT_H
#define INCLUDED_CONFIGS_ERROR_ASSERT_OUT_H

#include "configs/types/types_type.h"
#include "configs/error/common.h"

#include <tuple>
#include <type_traits>
#include <string>

namespace vt { namespace debug { namespace assert {

template <typename=void>
inline void assertOutExpr(
  bool fail, std::string const cond, std::string const& file, int const line,
  std::string const& func, ErrorCodeType error
);

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, Args... args
);

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, Args... args
);

}}} /* end namespace vt::debug::assert */

#include "configs/error/assert_out.impl.h"

#endif /*INCLUDED_CONFIGS_ERROR_ASSERT_OUT_H*/
