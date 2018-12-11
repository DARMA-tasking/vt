
#if !defined INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H
#define INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H

#include "vt/configs/error/common.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/assert_out.h"
#include "vt/configs/error/assert_out_info.h"
#include "vt/configs/error/keyval_printer.h"
#include "vt/configs/debug/debug_colorize.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <string>
#include <sstream>

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

  auto node      = debug::preNode();
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  auto node_str  = debug::proc(node);
  auto prefix    = vt_pre + node_str + " ";
  auto byellow   = debug::byellow();
  auto yellow    = debug::yellow();
  auto reset     = debug::reset();
  auto green     = debug::green();
  auto seperator = fmt::format("{}{}{:-^120}{}\n", prefix, yellow, "", reset);
  auto space     = fmt::format("{}\n", prefix);
  auto title     = fmt::format(
    "{}{}{:-^120}{}\n", prefix, yellow, " Debug State Assert Info ", reset
  );
  auto pre       = fmt::format("{}{}{}{}", seperator, title, seperator, space);

  std::ostringstream str_buf;

  auto buf = fmt::format("{}", pre);
  str_buf << buf;

  //str_buf << seperator << seperator;
  for (auto&& var : varlist) {
    auto cur = fmt::format("{}{}{:>25}{}\n", prefix, green, var, reset);
    str_buf << cur;
  }
  str_buf << space << seperator << seperator;
  ::vt::output(str_buf.str(),error,false,false,true);

  if (fail) {
    assert(false);
    std::exit(1);
  }
}

}}} /* end namespace vt::debug::assert */

#endif /*INCLUDED_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H*/
