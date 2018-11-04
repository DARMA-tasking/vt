
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_HASH_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_HASH_H

#include "vt/config.h"

#include <tuple>

namespace vt { namespace collective { namespace reduce {

using ReduceIdentifierType = std::tuple<TagType,EpochType,VirtualProxyType>;
using ReduceEpochLookupType = std::tuple<VirtualProxyType,TagType>;

}}} /* end namespace vt::collective::reduce */

namespace std {

using ReduceIDType = ::vt::collective::reduce::ReduceIdentifierType;
using ReduceLookupType = ::vt::collective::reduce::ReduceEpochLookupType;

template <>
struct hash<ReduceIDType> {
  size_t operator()(ReduceIDType const& in) const {
    auto const& combined =
      std::hash<vt::TagType>()(std::get<0>(in)) ^
      std::hash<vt::EpochType>()(std::get<1>(in)) ^
      std::hash<vt::VirtualProxyType>()(std::get<2>(in));
    return combined;
  }
};

template <>
struct hash<ReduceLookupType> {
  size_t operator()(ReduceLookupType const& in) const {
    auto const& combined =
      std::hash<vt::VirtualProxyType>()(std::get<0>(in)) ^
      std::hash<vt::TagType>()(std::get<1>(in));
    return combined;
  }
};

} /* end namespace std */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_HASH_H*/
