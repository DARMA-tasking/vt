
#if !defined INCLUDED_VRT_PROXY_PROXY_ELEMENT_H
#define INCLUDED_VRT_PROXY_PROXY_ELEMENT_H

#include "config.h"

#include <cstdlib>
#include <cstdint>

namespace vt { namespace vrt { namespace collection {

static struct virtual_proxy_elm_empty { } virtual_proxy_elm_empty_tag { };

template <typename IndexT>
struct VirtualProxyElementType {

  explicit VirtualProxyElementType(IndexT const& in_idx)
    : idx_(in_idx)
  { }
  explicit VirtualProxyElementType(virtual_proxy_elm_empty) { }

  VirtualProxyElementType() = default;
  VirtualProxyElementType(VirtualProxyElementType const&) = default;
  VirtualProxyElementType(VirtualProxyElementType&&) = default;
  VirtualProxyElementType& operator=(VirtualProxyElementType const&) = default;

  bool operator==(VirtualProxyElementType const& other) const {
    return other.idx_ == idx_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | idx_;
  }

private:
  IndexT const idx_;
};

}}} /* end namespace vt::vrt::collection */

template <typename IndexT>
using ElmType = ::vt::vrt::collection::VirtualProxyElementType<IndexT>;

namespace std {
  template <typename IndexT>
  struct hash<ElmType<IndexT>> {
    size_t operator()(ElmType<IndexT> const& in) const {
      return std::hash<decltype(in.colProxy)>()(in.idx_);
    }
  };
}

#endif /*INCLUDED_VRT_PROXY_PROXY_ELEMENT_H*/
