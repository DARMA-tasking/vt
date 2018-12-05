
#if !defined INCLUDED_VT_VRT_COLLECTION_HOLDERS_INSERT_CONTEXT_HOLDER_H
#define INCLUDED_VT_VRT_COLLECTION_HOLDERS_INSERT_CONTEXT_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct InsertContextHolder {

  static void set(IndexT* const set_idx, VirtualProxyType const& set_proxy) {
    ctx_idx = set_idx;
    ctx_proxy = set_proxy;
  }

  static void clear() {
    ctx_idx = nullptr;
    ctx_proxy = no_vrt_proxy;
  }

  static IndexT* index() {
    vtAssertExpr(ctx_idx != nullptr);
    return ctx_idx;
  }

  static VirtualProxyType proxy() {
    vtAssertExpr(ctx_proxy != no_vrt_proxy);
    return ctx_proxy;
  }

private:
  static IndexT* ctx_idx;
  static VirtualProxyType ctx_proxy;
};

template <typename IndexT>
/*static*/ IndexT* InsertContextHolder<IndexT>::ctx_idx = nullptr;

template <typename IndexT>
/*static*/ VirtualProxyType
InsertContextHolder<IndexT>::ctx_proxy = no_vrt_proxy;

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_HOLDERS_INSERT_CONTEXT_HOLDER_H*/
