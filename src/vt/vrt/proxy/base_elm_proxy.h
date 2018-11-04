
#if !defined INCLUDED_VRT_PROXY_BASE_ELM_PROXY_H
#define INCLUDED_VRT_PROXY_BASE_ELM_PROXY_H

#include "vt/config.h"

#include <cstdlib>
#include <cstdint>
#include <utility>

namespace vt { namespace vrt { namespace collection {

static struct virtual_proxy_elm_empty { } virtual_proxy_elm_empty_tag { };

template <typename ColT, typename IndexT>
struct BaseElmProxy {
  using IndexType = IndexT;
  using CollectionType = ColT;

  explicit BaseElmProxy(IndexT const& in_idx)
    : idx_(in_idx)
  { }
  explicit BaseElmProxy(virtual_proxy_elm_empty) { }

  BaseElmProxy() = default;
  BaseElmProxy(BaseElmProxy const&) = default;
  BaseElmProxy(BaseElmProxy&&) = default;
  BaseElmProxy& operator=(BaseElmProxy const&) = default;

  bool operator==(BaseElmProxy const& other) const {
    return other.idx_ == idx_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | idx_;
  }

  IndexT const& getIndex() const { return idx_; }

protected:
  IndexT idx_;
};

}}} /* end namespace vt::vrt::collection */

template <typename ColT, typename IndexT>
using ElmType = ::vt::vrt::collection::BaseElmProxy<ColT,IndexT>;

namespace std {
  template <typename ColT, typename IndexT>
  struct hash<ElmType<ColT,IndexT>> {
    size_t operator()(ElmType<ColT,IndexT> const& in) const {
      return std::hash<typename ElmType<ColT,IndexT>::IndexType>()(
        in.getIndex()
      );
    }
  };
}

#endif /*INCLUDED_VRT_PROXY_BASE_ELM_PROXY_H*/
