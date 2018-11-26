
#if !defined INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_H
#define INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT = typename ColT::IndexType>
struct InsertTokenRval {
  InsertTokenRval(VirtualProxyType const& in_proxy, IndexT&& in_idx)
    : proxy_(in_proxy),
      idx_(std::move(in_idx))
  { }
  InsertTokenRval(InsertTokenRval const&) = delete;
  InsertTokenRval(InsertTokenRval&&) = default;

  template <typename... Args>
  void insert(Args&&... args);

  friend CollectionManager;

private:
  VirtualProxyType proxy_ = no_vrt_proxy;
  IndexT idx_ = {};
};

template <typename ColT, typename IndexT = typename ColT::IndexType>
struct InsertToken {
private:
  InsertToken() = default;
  explicit InsertToken(VirtualProxyType const& in_proxy)
    : proxy_(in_proxy)
  { }

public:
  virtual ~InsertToken() = default;

public:
  InsertTokenRval<ColT> operator[](IndexT&& idx) {
    return InsertTokenRval<ColT>{proxy_,std::forward<IndexT>(idx)};
  }

  template <typename... IdxArgs>
  InsertTokenRval<ColT> operator[](IdxArgs&&... args) {
    using Base = typename IndexT::DenseIndexType;
    return InsertTokenRval<ColT>{proxy_,IndexT(static_cast<Base>(args)...)};
  }

  friend CollectionManager;

public:
  InsertToken(InsertToken const&) = delete;
  InsertToken(InsertToken&&) = default;

  VirtualProxyType getProxy() const { return proxy_; }

private:
  VirtualProxyType proxy_ = no_vrt_proxy;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_H*/
