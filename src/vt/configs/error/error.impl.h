
#if !defined INCLUDED_CONFIGS_ERROR_ERROR_IMPL_H
#define INCLUDED_CONFIGS_ERROR_ERROR_IMPL_H

#include "configs/debug/debug_config.h"
#include "configs/types/types_type.h"
#include "configs/error/common.h"
#include "configs/error/error.h"

#include <string>
#include <tuple>
#include <type_traits>

#include <fmt/format.h>

namespace vt { namespace error {

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
display(std::string const& str, ErrorCodeType error, Args&&... args) {
  std::string const inf = ::fmt::format("FATAL ERROR: {}\n",str);
  return ::vt::abort(inf,error);
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
display(std::string const& str, ErrorCodeType error, Args&&... args) {
  std::string const buf = ::fmt::format(str,std::forward<Args>(args)...);
  return ::vt::error::display(buf,error);
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
displayLoc(
  std::string const& str, ErrorCodeType error,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
) {
  std::string const inf = ::fmt::format(
    "FATAL ERROR: {}\nFile: {}\nLine: {}\nFunction: {}\n",str,file,line,func
  );
  return ::vt::abort(inf,error);
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
displayLoc(
  std::string const& str, ErrorCodeType error,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
) {
  std::string const buf = ::fmt::format(str,std::forward<Args>(args)...);
  return ::vt::error::displayLoc(buf,error,file,line,func);
}

}} /* end namespace vt::error */

#endif /*INCLUDED_CONFIGS_ERROR_ERROR_IMPL_H*/
