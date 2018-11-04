
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/holders/elm_holder.h"
#include "vt/vrt/collection/types/headers.h"
#include "vt/vrt/collection/messages/user.h"

#include <unordered_map>
#include <tuple>
#include <memory>
#include <functional>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Holder {
  template <typename T, typename U>
  using ContType = std::unordered_map<T, U>;
  using CollectionType = CollectionBase<ColT, IndexT>;
  using VirtualPtrType = std::unique_ptr<CollectionType>;
  using LookupElementType = IndexT;
  using InnerHolder = ElementHolder<ColT, IndexT>;
  using TypedIndexContainer = ContType<LookupElementType, InnerHolder>;
  using FuncApplyType = std::function<void(IndexT const&, CollectionType*)>;
  using FuncExprType = std::function<bool(IndexT const&)>;
  using CountType = uint64_t;

  bool exists(IndexT const& idx);
  InnerHolder& lookup(IndexT const& idx);
  void insert(IndexT const& idx, InnerHolder&& inner);
  VirtualPtrType remove(IndexT const& idx);
  void destroyAll();
  bool isDestroyed() const;
  bool foreach(FuncApplyType fn);
  typename TypedIndexContainer::size_type numElements() const;
  typename TypedIndexContainer::size_type numElementsExpr(FuncExprType f) const;
  GroupType group() const { return cur_group_; }
  void setGroup(GroupType const& group) { cur_group_ = group; }
  bool useGroup() const { return use_group_; }
  void setUseGroup(bool const use_group) { use_group_ = use_group; }
  bool groupReady() const { return group_ready_; }
  void setGroupReady(bool const ready) { group_ready_ = ready; }
  NodeType groupRoot() const { return group_root_; }
  void setGroupRoot(NodeType const root) { group_root_ = root; }

  friend struct CollectionManager;

private:
  bool erased                                                     = false;
  typename TypedIndexContainer::iterator foreach_iter             = {};
  std::unordered_map<EpochType, CollectionMessage<ColT>*> bcasts_ = {};
  EpochType cur_bcast_epoch_                                      = 1;
  TypedIndexContainer vc_container_                               = {};
  bool is_destroyed_                                              = false;
  GroupType cur_group_                                            = no_group;
  bool use_group_                                                 = false;
  bool group_ready_                                               = false;
  NodeType group_root_                                            = 0;
  CountType num_erased_not_removed_                               = 0;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H*/
