
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/collection.h"
#include "vrt/collection/collection_holder.h"
#include "topos/mapping/mapping_headers.h"

#include <memory>
#include <vector>
#include <tuple>
#include <utility>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection {

struct CollectionManager {
  template <typename IndexT>
  using CollectionType = typename CollectionHolder<IndexT>::Collection;

  template <typename IndexT>
  using VirtualPtrType = typename CollectionHolder<IndexT>::VirtualPtrType;

  CollectionManager() = default;

  template <
    typename CollectionT,
    typename IndexT,
    mapping::ActiveMapTypedFnType<IndexT> fn,
    typename... Args
  >
  VirtualProxyType makeCollection(IndexT const& range, Args&& ... args);

  template <typename SysMsgT>
  static void createCollectionHan(SysMsgT* msg);

  template <
    typename CollectionT,
    typename MessageT,
    ActiveCollectionTypedFnType<MessageT, CollectionT> *f
  >
  void sendMsg(
    VirtualElmProxyType const& toProxy, MessageT *const msg, ActionType act
  );

  template <typename IndexT>
  static void collectionMsgHandler(BaseMessage* msg);

  template <typename CollectionT, typename IndexT, typename Tuple, size_t... I>
  static VirtualPtrType<IndexT> runConstructor(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  );

  template <typename IndexT>
  void insertCollectionElement(
    VirtualPtrType<IndexT> vc, IndexT const& idx, IndexT const& max_idx,
    HandlerType const& map_han, VirtualProxyType const &proxy
  );

protected:
  VirtualProxyType makeNewCollectionProxy();
  void insertCollectionInfo(VirtualProxyType const& proxy);

private:
  VirtualIDType curIdent_ = 0;
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

extern vrt::collection::CollectionManager* theCollection();

}  // end namespace vt

#include "vrt/collection/collection_manager.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_H*/
