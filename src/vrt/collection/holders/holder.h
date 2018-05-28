
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/manager.fwd.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/holders/elm_holder.h"
#include "vrt/collection/types/headers.h"

#include <unordered_map>
#include <tuple>
#include <memory>
#include <functional>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Holder {
  template <typename T, typename U>
  using ContType = std::unordered_map<T, U>;
  using CollectionType = Collection<ColT, IndexT>;
  using VirtualPtrType = std::unique_ptr<CollectionType>;
  using LookupElementType = IndexT;
  using InnerHolder = ElementHolder<ColT, IndexT>;
  using TypedIndexContainer = ContType<LookupElementType, InnerHolder>;
  using FuncApplyType = std::function<void(IndexT const&, void*)>;

  bool exists(IndexT const& idx);
  InnerHolder& lookup(IndexT const& idx);
  void insert(IndexT const& idx, InnerHolder&& inner);
  VirtualPtrType remove(IndexT const& idx);
  void destroyAll();
  bool isDestroyed() const;
  bool foreach(FuncApplyType fn);

  friend struct CollectionManager;

private:
  TypedIndexContainer vc_container_;
  bool is_destroyed_ = false;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/holders/holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H*/
