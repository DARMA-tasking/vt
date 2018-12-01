
#if !defined INCLUDED_CONFIGS_ERROR_ERROR_IMPL_H
#define INCLUDED_CONFIGS_ERROR_ERROR_IMPL_H

#include "vt/configs/debug/debug_config.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/common.h"
#include "vt/configs/error/error.h"
#include "vt/configs/error/pretty_print_message.h"

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
  auto msg = "vtAbort() Invoked";
  auto const inf = debug::stringizeMessage(msg,str,"",file,line,func,error);
  return ::vt::output(inf,error,true,true,true);
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
displayLoc(
  std::string const& str, ErrorCodeType error,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
) {
  auto msg = "vtAbort() Invoked";
  std::string const buf = ::fmt::format(str,std::forward<Args>(args)...);
  auto const inf = debug::stringizeMessage(msg,buf,"",file,line,func,error);
  return ::vt::output(inf,error,true,true,true);
}

}} /* end namespace vt::error */

#endif /*INCLUDED_CONFIGS_ERROR_ERROR_IMPL_H*/
