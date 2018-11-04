
#if !defined INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H
#define INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H

#include "vt/configs/error/common.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/assert_out.h"
#include "vt/configs/error/assert_out_info.h"
#include "vt/configs/error/keyval_printer.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <string>

#include <fmt/format.h>

namespace vt { namespace debug { namespace assert {

template <typename... Args, typename... Args2>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
assertOutInfo(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args2...> tup, Args... args
) {
  return assertOut(fail,cond,str,file,line,func,error,args...);
}

template <typename... Args, typename... Args2>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOutInfo(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args2...> t1, Args... args
) {
  using KeyType = std::tuple<Args2...>;
  using ValueType = std::tuple<Args...>;
  static constexpr auto size = std::tuple_size<KeyType>::value;
  using PrinterType = util::error::PrinterNameValue<size-1,KeyType,ValueType>;

  // Output the standard assert message
  assertOut(false,cond,str,file,line,func,error);

  // Output each expression computed passed to the function along with the
  // computed value of that passed expression
  auto const t2 = std::make_tuple(args...);
  auto varlist = PrinterType::make(t1,t2);

  std::string state = ::fmt::format("{:*^80}\n\n", " DEBUG STATE ");
  for (auto&& var : varlist) {
    state += var + "\n";
  }
  state += "\n";
  state += ::fmt::format("{:*^80}\n", "");
  state += "\n";
  ::vt::output(state,error,false,false);

  if (fail) {
    assert(false);
  }
}

}}} /* end namespace vt::debug::assert */

#endif /*INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H*/
