
#if !defined INCLUDED_CONFIGS_ERROR_KEYVAL_PRINTER_H
#define INCLUDED_CONFIGS_ERROR_KEYVAL_PRINTER_H

#include "vt/configs/error/common.h"
#include "vt/configs/debug/debug_config.h"

#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <string>
#include <vector>

namespace vt { namespace util { namespace error {

template <std::size_t cur, typename ConsT, typename ConsU>
struct PrinterNameValue;

template <typename ConsT, typename ConsU>
struct PrinterNameValue<0,ConsT,ConsU> {
  using ResultType = std::tuple<>;
  using VectorType = std::vector<std::string>;
  static VectorType make(ConsT const& names, ConsU const& values);
};

template <std::size_t cur, typename ConsT, typename ConsU>
struct PrinterNameValue {
  using VectorType = std::vector<std::string>;
  static VectorType make(ConsT const& names, ConsU const& values);
};

}}} /* end namespace vt::util::error */

#include "vt/configs/error/keyval_printer.impl.h"

#endif /*INCLUDED_CONFIGS_ERROR_KEYVAL_PRINTER_H*/
