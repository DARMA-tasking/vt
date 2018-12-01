
#if !defined INCLUDED_CONFIGS_ERROR_KEYVAL_PRINTER_IMPL_H
#define INCLUDED_CONFIGS_ERROR_KEYVAL_PRINTER_IMPL_H

#include "vt/configs/error/keyval_printer.h"
#include "vt/configs/debug/debug_colorize.h"

#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace vt { namespace util { namespace error {

template <typename T, typename U>
static std::string makeVal(T const& t, U const& u) {
  auto green           = ::vt::debug::green();
  auto blue            = ::vt::debug::blue();
  auto reset           = ::vt::debug::reset();
  auto bred            = ::vt::debug::bred();
  auto magenta         = ::vt::debug::magenta();
  return ::fmt::format("{}{:>40}{} = {}{}{}",magenta,t,reset,green,u,reset);
}

template <typename ConsT, typename ConsU>
/*static*/ typename PrinterNameValue<0,ConsT,ConsU>::VectorType
PrinterNameValue<0,ConsT,ConsU>::make(ConsT const& names, ConsU const& values) {
  return VectorType{makeVal(std::get<0>(names),std::get<0>(values))};
}

template <std::size_t cur, typename ConsT, typename ConsU>
/*static*/ typename PrinterNameValue<cur,ConsT,ConsU>::VectorType
PrinterNameValue<cur,ConsT,ConsU>::make(ConsT const& names, ConsU const& values) {
  auto vec = PrinterNameValue<cur-1,ConsT,ConsU>::make(names,values);
  vec.emplace_back(makeVal(std::get<cur>(names),std::get<cur>(values)));
  return vec;
}

}}} /* end namespace vt::util::error */

#endif /*INCLUDED_CONFIGS_ERROR_KEYVAL_PRINTER_IMPL_H*/
