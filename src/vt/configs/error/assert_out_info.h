
#if !defined INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_H
#define INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_H

#include "configs/error/common.h"
#include "configs/types/types_type.h"

#include <tuple>
#include <string>

namespace vt { namespace debug { namespace assert {

template <typename... Args, typename... Args2>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
assertOutInfo(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args2...> tup, Args... args
);

template <typename... Args, typename... Args2>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOutInfo(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args2...> t1, Args... args
);

}}} /* end namespace vt::debug::assert */

#include "configs/error/assert_out_info.impl.h"

#endif /*INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_H*/
