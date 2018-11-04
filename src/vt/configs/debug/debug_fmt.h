
#if !defined INCLUDED_CONFIGS_DEBUG_DEBUG_FMT_H
#define INCLUDED_CONFIGS_DEBUG_DEBUG_FMT_H

#include "config.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <ostream>
#include <type_traits>

namespace vt { namespace debug {

template <typename EnumT>
inline std::ostream& printEnum(std::ostream& os, EnumT const& val) {
  using UnderT = typename std::underlying_type<EnumT>::type;
  auto const num = static_cast<UnderT>(val);
  os << "enum(" << static_cast<int32_t>(num) << ")";
  return os;
}

}} /* end namespace vt::debug */

#endif /*INCLUDED_CONFIGS_DEBUG_DEBUG_FMT_H*/
