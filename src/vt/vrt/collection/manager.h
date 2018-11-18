
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_H
#define INCLUDED_VRT_COLLECTION_MANAGER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/messages/system_create.h"
#include "vt/vrt/collection/types/headers.h"
#include "vt/vrt/collection/holders/holder.h"
#include "vt/vrt/collection/holders/entire_holder.h"
#include "vt/vrt/collection/traits/cons_detect.h"
#include "vt/vrt/collection/traits/cons_dispatch.h"
#include "vt/vrt/collection/constructor/coll_constructors.h"
#include "vt/vrt/collection/migrate/manager_migrate_attorney.fwd.h"
#include "vt/vrt/collection/migrate/migrate_status.h"
#include "vt/vrt/collection/destroy/manager_destroy_attorney.fwd.h"
#include "vt/vrt/collection/messages/user_wrap.h"
#include "vt/vrt/collection/traits/coll_msg.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/messaging/message.h"
#include "vt/topos/location/location_headers.h"
#include "vt/collective/collective_alg.h"
#include "vt/collective/reduce/reduce_msg.h"
#include "vt/collective/reduce/reduce_hash.h"

#include <memory>
#include <vector>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <vector>
#include <list>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection {

struct CollectionManager {
  template <typename ColT, typename IndexT>
  using CollectionType = typename Holder<ColT, IndexT>::Collection;
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  using VirtualPtrType = typename Holder<ColT, IndexT>::VirtualPtrType;
  using ActionProxyType = std::function<void(VirtualProxyType)>;
  using ActionFinishedLBType = std::function<void()>;
  using NoElementActionType = std::function<void()>;
  template <typename IndexT>
  using ReduceIdxFuncType = std::function<bool(IndexT const&)>;
  using ActionContainerType = std::vector<ActionProxyType>;
  using BufferedActionType = std::unordered_map<
    VirtualProxyType, ActionContainerType
  >;
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  using CollectionProxyWrapType = CollectionProxy<ColT,IndexT>;
  using ReduceIDType = ::vt::collective::reduce::ReduceEpochLookupType;
  template <typename ColT>
  using EpochBcastType = std::unordered_map<EpochType,CollectionMessage<ColT>*>;
  template <typename ColT>
  using BcastBufferType = std::unordered_map<
    VirtualProxyType, EpochBcastType<ColT>
  >;
  using CleanupFnType = std::function<void()>;
  using CleanupListFnType = std::list<CleanupFnType>;
  using DispatchHandlerType = auto_registry::AutoHandlerType;
  using ActionVecType = std::vector<ActionType>;

  template <typename ColT, typename IndexT = typename ColT::IndexType>
  using DistribConstructFn = std::function<VirtualPtrType<ColT>(IndexT idx)>;

  template <typename T, typename U=void>
  using IsColMsgType = std::enable_if_t<ColMsgTraits<T>::is_coll_msg>;
  template <typename T, typename U=void>
  using IsNotColMsgType = std::enable_if_t<!ColMsgTraits<T>::is_coll_msg>;

  template <typename ColT, typename UserMsgT, typename T, typename U=void>
  using IsWrapType = std::enable_if_t<
    std::is_same<T,ColMsgWrap<ColT,UserMsgT>>::value,U
  >;
  template <typename ColT, typename UserMsgT, typename T, typename U=void>
  using IsNotWrapType = std::enable_if_t<
    !std::is_same<T,ColMsgWrap<ColT,UserMsgT>>::value,U
  >;

  CollectionManager() = default;

  virtual ~CollectionManager() { cleanupAll<>(); }

  template <typename=void>
  void cleanupAll();

  template <typename=void>
  void destroyCollections();

  /*
   *         CollectionManager::constructMap<ColT, Args...>
   *
   *  Construct virtual context collection with an initial pre-registered map
   *  function.
   */
  template <typename ColT, typename... Args>
  CollectionProxyWrapType<ColT, typename ColT::IndexType>
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
  CollectionProxyWrapType<ColT, typename ColT::IndexType>
  construct(typename ColT::IndexType range, Args&&... args);

  /*
   *      CollectionManager::construct<ColT, Args...>
   *
   *  Construct virtual context collection using the default map for the given
   *  index. Found by looking up a vrt::collection::DefaultMap<...>
   *  specialization for the Index type.
   */
  template <typename ColT, typename... Args>
  CollectionProxyWrapType<ColT, typename ColT::IndexType>
  construct(typename ColT::IndexType range, Args&&... args);

  /*
   *      CollectionManager::construct<ColT, MapFnT>
   *
   *  Construct virtual context collection with an explicit map. This construct
   *  method enables distributed SPMD construction of the virtual context
   *  collection using the `DistribConstructFn`. The system will invoke that
   *  function for every index in the system based on the where each index is
   *  mapped with the MapFnT.
   */
  template <
    typename ColT,  mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
  >
  CollectionProxyWrapType<ColT, typename ColT::IndexType>
  construct(typename ColT::IndexType range, DistribConstructFn<ColT> cons_fn);

  /*
   * Private interface for distConstruct that CollectionManager uses to
   * broadcast and construct on every node.
   */

  template <typename SysMsgT>
  static void distConstruct(SysMsgT* msg);

  /*
   *  Send message to an element of a collection
   */
  template <
    typename MsgT,
    typename ColT = typename MsgT::CollectionType,
    typename IdxT = typename ColT::IndexType
  >
  void sendMsgUntypedHandler(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member, ActionType action
  );

  template <typename MsgT, typename ColT>
  IsNotColMsgType<MsgT> sendMsgWithHan(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member, ActionType action
  );

  template <typename MsgT, typename ColT>
  IsColMsgType<MsgT> sendMsgWithHan(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member, ActionType action
  );

  template <typename MsgT, typename ColT>
  void sendNormalMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member, ActionType action
  );

  template <
    typename MsgT, ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
  >
  void sendMsg(
    VirtualElmProxyType<typename MsgT::CollectionType> const& proxy, MsgT *msg,
    ActionType act = nullptr
  );

  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
  >
  void sendMsg(
    VirtualElmProxyType<typename MsgT::CollectionType> const& proxy, MsgT *msg,
    ActionType act = nullptr
  );

  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg, ActionType act = nullptr
  );

  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsNotColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg, ActionType act = nullptr
  );

  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg, ActionType act = nullptr
  );

  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsNotColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg, ActionType act = nullptr
  );

  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  void sendMsgImpl(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg, ActionType act = nullptr
  );

  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
  >
  void sendMsgImpl(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg, ActionType act = nullptr
  );

  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
  static IsWrapType<ColT,UserMsgT,MsgT> collectionMsgDeliver(
    MsgT* msg, CollectionBase<ColT,IndexT>* col, HandlerType han, bool member,
    NodeType from
  );
  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
  static IsNotWrapType<ColT,UserMsgT,MsgT> collectionMsgDeliver(
    MsgT* msg, CollectionBase<ColT,IndexT>* col, HandlerType han, bool member,
    NodeType from
  );

  template <typename CoLT, typename IndexT>
  static void collectionMsgHandler(BaseMessage* msg);

  template <typename ColT, typename IndexT, typename MsgT>
  static void collectionMsgTypedHandler(MsgT* msg);

  /*
   *  Reduce all elements of a collection
   */
  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceMsg(
    CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
    MsgT *const msg, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag,
    NodeType const& root_node = uninitialized_destination
  );

  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceMsg(
    CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
    MsgT *const msg, EpochType const& epoch, TagType const& tag,
    typename ColT::IndexType const& idx
  );

  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceMsgExpr(
    CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
    MsgT *const msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
    EpochType const& epoch = no_epoch, TagType const& tag = no_tag,
    NodeType const& root_node = uninitialized_destination
  );

  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceMsgExpr(
    CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
    MsgT *const msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
    EpochType const& epoch, TagType const& tag,
    typename ColT::IndexType const& idx
  );

  /*
   *  Broadcast message to all elements of a collection
   */
  template <typename MsgT, typename ColT>
  IsNotColMsgType<MsgT> broadcastMsgWithHan(
    CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member,
    ActionType act = nullptr, bool instrument = true
  );

  template <typename MsgT, typename ColT>
  IsColMsgType<MsgT> broadcastMsgWithHan(
    CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member,
    ActionType act = nullptr, bool instrument = true
  );

  template <typename MsgT, typename ColT>
  void broadcastNormalMsg(
    CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member,
    ActionType act = nullptr, bool instrument = true
  );

  template <typename MsgT, typename ColT, typename IdxT>
  void broadcastMsgUntypedHandler(
    CollectionProxyWrapType<ColT,IdxT> const& proxy, MsgT *msg,
    HandlerType const& handler, bool const member,
    ActionType act, bool instrument
  );

  // CollectionMessage non-detecting broadcast
  template <
    typename MsgT,
    ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
  >
  void broadcastMsg(
    CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
  >
  void broadcastMsg(
    CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsNotColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsNotColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  void broadcastMsgImpl(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  void broadcastMsgImpl(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, ActionType act = nullptr, bool instrument = true
  );

  template <typename ColT, typename IndexT, typename MsgT>
  void broadcastFromRoot(MsgT* msg);

public:
  /*
   * Vrt collection type/handle registration for typeless dispatch
   */
  template <typename MsgT, typename ColT>
  DispatchHandlerType getDispatchHandler();
  template <typename=void>
  DispatchBasePtrType getDispatcher(DispatchHandlerType const& han);

private:
  template <typename ColT, typename MsgT>
  void bufferBroadcastMsg(
    VirtualProxyType const& proxy, EpochType const& epoch, MsgT* msg
  );

  template <typename ColT>
  void clearBufferedBroadcastMsg(
    VirtualProxyType const& proxy, EpochType const& epoch
  );

  template <typename ColT, typename MsgT>
  CollectionMessage<ColT>* getBufferedBroadcastMsg(
    VirtualProxyType const& proxy, EpochType const& epoch
  );

public:
  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
  static IsWrapType<ColT,UserMsgT,MsgT> collectionAutoMsgDeliver(
    MsgT* msg, CollectionBase<ColT,IndexT>* col, HandlerType han, bool member,
    NodeType from
  );
  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
  static IsNotWrapType<ColT,UserMsgT,MsgT> collectionAutoMsgDeliver(
    MsgT* msg, CollectionBase<ColT,IndexT>* col, HandlerType han, bool member,
    NodeType from
  );

  template <typename ColT, typename IndexT, typename MsgT>
  static void collectionBcastHandler(MsgT* msg);
  template <typename ColT, typename IndexT, typename MsgT>
  static void broadcastRootHandler(MsgT* msg);
  template <typename=void>
  static void collectionConstructHan(CollectionConsMsg* msg);
  template <typename=void>
  static void collectionFinishedHan(CollectionConsMsg* msg);
  template <typename=void>
  static void collectionGroupReduceHan(CollectionGroupMsg* msg);
  template <typename=void>
  static void collectionGroupFinishedHan(CollectionGroupMsg* msg);

  /*
   *  Automatic group creation for each collection instance for broadcasts
   *  (optimization) and reduce (correctness)
   */

  template <typename ColT, typename IndexT>
  std::size_t groupElementCount(VirtualProxyType const& proxy);

  template <typename ColT, typename IndexT>
  GroupType createGroupCollection(
    VirtualProxyType const& proxy, bool const in_group
  );


  /*
   * Traits version of running the constructor based on the detected available
   * constructor types:
   *
   * This variant is accessed through the traits class directly and the
   * constructor is invoked
   */

  /*
   * Non-traits version of running the constructor: does not require the
   * detection idiom to dispatch to constructor.
   */

  template <typename ColT, typename IndexT, typename Tuple, size_t... I>
  static VirtualPtrType<ColT, IndexT> runConstructor(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  );

  template <typename ColT, typename IndexT = typename ColT::IndexType>
  bool insertCollectionElement(
    VirtualPtrType<ColT, IndexT> vc, IndexT const& idx, IndexT const& max_idx,
    HandlerType const& map_han, VirtualProxyType const& proxy,
    bool const is_static, NodeType const& home_node,
    bool const& is_migrated_in = false,
    NodeType const& migrated_from = uninitialized_destination
  );

  /*
   *  LB-related operations on the collection
   */
  template <typename ColT>
  void nextPhase(
    CollectionProxyWrapType<ColT, typename ColT::IndexType> const& proxy,
    PhaseType const& cur_phase, ActionFinishedLBType continuation = nullptr
  );

  template <typename ColT>
  void computeStats(
    CollectionProxyWrapType<ColT, typename ColT::IndexType> const& proxy,
    PhaseType const& cur_phase
  );

  template <typename=void>
  static void releaseLBPhase(CollectionPhaseMsg* msg);

  template <typename=void>
  std::size_t numCollections();

  template <typename=void>
  std::size_t numReadyCollections();

  template <typename=void>
  bool readyNextPhase();

  template <typename=void>
  void resetReadyPhase();

  template <typename=void>
  void releaseLBContinuation();

  template <typename=void>
  void makeCollectionReady(VirtualProxyType const coll);

  template <typename=void>
  void checkReduceNoElements();

private:
  template <typename ColT, typename IndexT>
  CollectionHolder<ColT, IndexT>* findColHolder(VirtualProxyType const& proxy);
  template <typename ColT, typename IndexT>
  Holder<ColT, IndexT>* findElmHolder(VirtualProxyType const& proxy);

public:
  template <typename ColT, typename IndexT>
  void destroy(CollectionProxyWrapType<ColT,IndexT> const& proxy);

private:
  template <typename ColT, typename IndexT>
  void incomingDestroy(CollectionProxyWrapType<ColT,IndexT> const& proxy);

  template <typename ColT, typename IndexT>
  void destroyMatching(CollectionProxyWrapType<ColT,IndexT> const& proxy);

protected:
  VirtualProxyType makeNewCollectionProxy();
  void insertCollectionInfo(
    VirtualProxyType const& proxy, HandlerType const& map,
    EpochType const& insert_epoch = no_epoch
  );

public:
  template <typename ColT, typename IndexT>
  NodeType getMappedNode(
    CollectionProxyWrapType<ColT,IndexT> const& proxy,
    typename ColT::IndexType const& idx
  );

public:
  template <typename ColT>
  MigrateStatus migrate(
    VrtElmProxy<ColT, typename ColT::IndexType>, NodeType const& dest
  );

  template <typename ColT, typename IndexT>
  static void insertHandler(InsertMsg<ColT,IndexT>* msg);

  template <typename ColT, typename IndexT = typename ColT::IndexType>
  void insert(
    CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx,
    NodeType const& node = uninitialized_destination
  );

  template <typename ColT, typename IndexT = typename ColT::IndexType>
  ColT* tryGetLocalPtr(
    CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx
  );

  template <typename ColT, typename IndexT>
  static void doneInsertHandler(DoneInsertMsg<ColT,IndexT>* msg);

  template <typename ColT, typename IndexT>
  static void actInsertHandler(ActInsertMsg<ColT,IndexT>* msg);

  template <typename ColT, typename IndexT>
  static void updateInsertEpochHandler(UpdateInsertMsg<ColT,IndexT>* msg);

  template <typename=void>
  static void finishedUpdateHan(FinishedUpdateMsg* msg);

  template <typename=void>
  void actInsert(VirtualProxyType const& proxy);

  template <typename ColT, typename IndexT>
  void setupNextInsertTrigger(
    VirtualProxyType const& proxy, EpochType const& insert_epoch
  );

  template <typename ColT, typename IndexT = typename ColT::IndexType>
  void finishedInserting(
    CollectionProxyWrapType<ColT,IndexT> const& proxy,
    ActionType insert_action = nullptr
  );

private:
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  void finishedInsertEpoch(
    CollectionProxyWrapType<ColT,IndexT> const& proxy,
    EpochType const& insert_epoch
  );

private:
  template <typename ColT, typename IndexT>
  friend struct CollectionElmAttorney;

  template <typename ColT, typename IndexT>
  friend struct CollectionElmDestroyAttorney;

  friend struct balance::ElementStats;

  template <typename ColT, typename IndexT>
  MigrateStatus migrateOut(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& dest
  );

  template <typename ColT, typename IndexT>
  MigrateStatus migrateIn(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& from,
    VirtualPtrType<ColT, IndexT> vrt_elm_ptr, IndexT const& range,
    HandlerType const& map_han
  );

private:
  template <typename=void>
  static VirtualIDType curIdent_;

  template <typename ColT>
  static BcastBufferType<ColT> broadcasts_;

  CleanupListFnType cleanup_fns_;
  BufferedActionType buffered_sends_;
  BufferedActionType buffered_bcasts_;
  BufferedActionType buffered_group_;
  std::unordered_set<VirtualProxyType> constructed_;
  std::unordered_map<ReduceIDType,EpochType> reduce_cur_epoch_;
  std::vector<ActionFinishedLBType> lb_continuations_ = {};
  std::unordered_map<VirtualProxyType,NoElementActionType> lb_no_elm_ = {};
  std::unordered_map<VirtualProxyType,ActionVecType> insert_finished_action_ = {};
  std::unordered_map<VirtualProxyType,ActionVecType> user_insert_action_ = {};
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

extern vrt::collection::CollectionManager* theCollection();

}  // end namespace vt

#include "vt/vrt/collection/manager.impl.h"
#include "vt/vrt/collection/migrate/manager_migrate_attorney.impl.h"
#include "vt/vrt/collection/send/sendable.impl.h"
#include "vt/vrt/collection/gettable/gettable.impl.h"
#include "vt/vrt/collection/reducable/reducable.impl.h"
#include "vt/vrt/collection/insert/insertable.impl.h"
#include "vt/vrt/collection/insert/insert_finished.impl.h"
#include "vt/vrt/collection/destroy/destroyable.impl.h"
#include "vt/vrt/collection/destroy/manager_destroy_attorney.impl.h"
#include "vt/vrt/collection/broadcast/broadcastable.impl.h"
#include "vt/vrt/collection/balance/elm_stats.impl.h"
#include "vt/vrt/collection/types/insertable.impl.h"
#include "vt/vrt/collection/dispatch/dispatch.impl.h"
#include "vt/vrt/collection/dispatch/registry.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_H*/
