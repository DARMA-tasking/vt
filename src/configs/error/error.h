
#if !defined INCLUDED_CONFIGS_ERROR_ERROR_H
#define INCLUDED_CONFIGS_ERROR_ERROR_H

#include "configs/debug/debug_config.h"
#include "configs/types/types_type.h"
#include "configs/error/common.h"

#include <string>
#include <tuple>
#include <type_traits>

namespace vt { namespace error {

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
display(std::string const& str, ErrorCodeType error, Args&&... args);

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
display(std::string const& str, ErrorCodeType error, Args&&... args);

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
displayLoc(
  std::string const& str, ErrorCodeType error,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
);

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
displayLoc(
  std::string const& str, ErrorCodeType error,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
);

}} /* end namespace vt::error */

#include "configs/error/error.impl.h"

#endif /*INCLUDED_CONFIGS_ERROR_ERROR_H*/
