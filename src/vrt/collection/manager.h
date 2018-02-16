
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_H
#define INCLUDED_VRT_COLLECTION_MANAGER_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/manager.fwd.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/types/headers.h"
#include "vrt/collection/holders/holder.h"
#include "vrt/collection/holders/entire_holder.h"
#include "vrt/proxy/collection_wrapper.h"
#include "topos/mapping/mapping_headers.h"
#include "messaging/message.h"
#include "topos/location/location_headers.h"

#include <memory>
#include <vector>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <functional>
#include <vector>

namespace vt { namespace vrt { namespace collection {

struct CollectionManager {
  template <typename IndexT>
  using CollectionType = typename Holder<IndexT>::Collection;
  template <typename IndexT>
  using VirtualPtrType = typename Holder<IndexT>::VirtualPtrType;
  using ActionProxyType = std::function<void(VirtualProxyType)>;
  using ActionContainerType = std::vector<ActionProxyType>;
  using BufferedActionType = std::unordered_map<
    VirtualProxyType, ActionContainerType
  >;
  template <typename IndexT>
  using CollectionProxyWrapType = CollectionIndexProxy<IndexT>;

  CollectionManager() = default;

  /*
   *         CollectionManager::constructMap<ColT, Args...>
   *
   *  Construct virtual context collection with an initial pre-registered map
   *  function.
   */
  template <typename ColT, typename... Args>
  CollectionProxyWrapType<typename ColT::IndexType>
  constructMap(
    typename ColT::IndexType range, HandlerType const& map,
    Args&&... args
  );

  /*
   *      CollectionManager::construct<ColT, MapFnT, Args...>
   *
   *  Construct virtual context collection with an explicit templated map
   *  function, causing registration to occur.
   */
  template <
    typename ColT, mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn,
    typename... Args
  >
  CollectionProxyWrapType<typename ColT::IndexType>
  construct(typename ColT::IndexType range, Args&&... args);

  /*
   *      CollectionManager::construct<ColT, Args...>
   *
   *  Construct virtual context collection using the default map for the given
   *  index. Found by looking up a vrt::collection::DefaultMap<...>
   *  specialization for the Index type.
   */
  template <typename ColT, typename... Args>
  CollectionProxyWrapType<typename ColT::IndexType>
  construct(typename ColT::IndexType range, Args&&... args);

  template <typename SysMsgT>
  static void distConstruct(SysMsgT* msg);

  template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  void sendMsg(
    VirtualElmProxyType<typename ColT::IndexType> const& toProxy,
    MsgT *const msg, ActionType act
  );

  template <typename IndexT>
  static void collectionMsgHandler(BaseMessage* msg);

  template <typename ColT, typename IndexT, typename Tuple, size_t... I>
  static VirtualPtrType<IndexT> runConstructor(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  );

  template <typename IndexT>
  void insertCollectionElement(
    VirtualPtrType<IndexT> vc, IndexT const& idx, IndexT const& max_idx,
    HandlerType const& map_han, VirtualProxyType const& proxy,
    int const& lm_inst_id
  );

protected:
  VirtualProxyType makeNewCollectionProxy();
  void insertCollectionInfo(VirtualProxyType const& proxy);

private:
  BufferedActionType buffered_sends_;
  VirtualIDType curIdent_ = 0;
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

extern vrt::collection::CollectionManager* theCollection();

}  // end namespace vt

#include "vrt/collection/manager.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_H*/
