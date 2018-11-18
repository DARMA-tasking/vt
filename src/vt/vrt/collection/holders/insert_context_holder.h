
#if !defined INCLUDED_VT_VRT_COLLECTION_HOLDERS_INSERT_CONTEXT_HOLDER_H
#define INCLUDED_VT_VRT_COLLECTION_HOLDERS_INSERT_CONTEXT_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct InsertContextHolder {

  static void set(IndexT* const set_idx) {
    ctx_idx = set_idx;
  }

  static void clear() {
    ctx_idx = nullptr;
  }

  static IndexT* index() {
    vtAssertExpr(ctx_idx != nullptr);
    return ctx_idx;
  }

private:
  static IndexT* ctx_idx;
};

template <typename IndexT>
/*static*/ IndexT* InsertContextHolder<IndexT>::ctx_idx = nullptr;

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_HOLDERS_INSERT_CONTEXT_HOLDER_H*/
