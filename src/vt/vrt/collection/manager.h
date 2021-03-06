/*
//@HEADER
// *****************************************************************************
//
//                                  manager.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

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
#include "vt/vrt/collection/staged_token/token.h"
#include "vt/vrt/collection/listener/listen_events.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/messaging/message.h"
#include "vt/messaging/pending_send.h"
#include "vt/topos/location/location_headers.h"
#include "vt/collective/collective_alg.h"
#include "vt/collective/reduce/reduce_msg.h"
#include "vt/collective/reduce/reduce_hash.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/vrt/collection/op_buffer.h"
#include "vt/runnable/invoke.h"

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

/**
 * \struct CollectionManager
 *
 * \brief A core VT component managing collections of tasks across the system.
 *
 * The collection manager enables the creation, execution, message routing, and
 * destruction of multi-dimensional collections of virtual contexts that are
 * mapped to hardware resources. Dense, sparse, on-demand collection types are
 * all supported. The location of these virtual contexts is managed by the
 * location manager as they migrate or the load balancer is invoked, which moves
 * them around the system based on instrumentation.
 */
struct CollectionManager
  : runtime::component::Component<CollectionManager>
{
  template <typename ColT, typename IndexT>
  using CollectionType = typename Holder<ColT, IndexT>::Collection;
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  using VirtualPtrType = typename Holder<ColT, IndexT>::VirtualPtrType;
  using ActionProxyType = std::function<void(VirtualProxyType)>;
  template <typename IndexT>
  using ReduceIdxFuncType = std::function<bool(IndexT const&)>;
  using ReduceVirtualIDType = collective::reduce::ReduceVirtualIDType;
  using ReduceStamp = collective::reduce::ReduceStamp;
  using ActionContainerType = std::vector<ActionProxyType>;
  using BufferedActionType = std::unordered_map<
    VirtualProxyType, ActionContainerType
  >;
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  using CollectionProxyWrapType = CollectionProxy<ColT,IndexT>;
  template <typename ColT>
  using EpochBcastType = std::unordered_map<EpochType,CollectionMessage<ColT>*>;
  template <typename ColT>
  using BcastBufferType = std::unordered_map<
    VirtualProxyType, EpochBcastType<ColT>
  >;
  using CleanupFnType = std::function<void()>;
  using CleanupListFnType = std::unordered_map<VirtualProxyType,std::list<CleanupFnType>>;
  using DispatchHandlerType = auto_registry::AutoHandlerType;
  using ActionVecType = std::vector<ActionType>;

  template <typename ColT, typename IndexT = typename ColT::IndexType>
  using DistribConstructFn = std::function<VirtualPtrType<ColT>(IndexT idx)>;

  template <typename T, typename U=void>
  using IsColMsgType = std::enable_if_t<ColMsgTraits<T>::is_coll_msg, messaging::PendingSend>;
  template <typename T, typename U=void>
  using IsNotColMsgType = std::enable_if_t<!ColMsgTraits<T>::is_coll_msg, messaging::PendingSend>;

  template <typename ColT, typename IndexT = typename ColT::IndexType>
  using IsDefaultConstructableType = std::enable_if_t<
    std::is_default_constructible<ColT>::value, CollectionProxyWrapType<ColT, IndexT>
  >;

  template <typename ColT, typename UserMsgT, typename T, typename U=void>
  using IsWrapType = std::enable_if_t<
    std::is_same<T,ColMsgWrap<ColT,UserMsgT>>::value,U
  >;
  template <typename ColT, typename UserMsgT, typename T, typename U=void>
  using IsNotWrapType = std::enable_if_t<
    !std::is_same<T,ColMsgWrap<ColT,UserMsgT>>::value,U
  >;

  /**
   * \internal \brief System call to construct a collection manager
   */
  CollectionManager();

  virtual ~CollectionManager();

  void startup() override;

  void finalize() override;

  std::string name() override { return "CollectionManager"; }

  /**
   * \internal \brief Trigger cleanup lambdas---triggered when termination
   * occurs
   */
  template <typename=void>
  void cleanupAll();

  /**
   * \internal \brief Destroy all collections
   */
  template <typename=void>
  void destroyCollections();

  /**
   * \brief Construct a new virtual context collection with an explicit,
   * pre-registered map handler
   *
   * \param[in] range index range for the collection
   * \param[in] map pre-registered map handler
   * \param[in] args arguments to pass to collection construct
   *
   * \return proxy to the new collection
   */
  template <typename ColT, typename... Args>
  CollectionProxyWrapType<ColT, typename ColT::IndexType>
  constructMap(
    typename ColT::IndexType range, HandlerType const map,
    Args&&... args
  );

  /**
   * \brief Construct a new virtual context collection with templated map
   *
   * \param[in] range index range for the collection
   * \param[in] args arguments to pass to collection construct
   *
   * \return proxy to the new collection
   */
  template <
    typename ColT, mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn,
    typename... Args
  >
  CollectionProxyWrapType<ColT, typename ColT::IndexType>
  construct(typename ColT::IndexType range, Args&&... args);

  /**
   * \brief Construct a new virtual context collection using the default map for
   *  the given index.
   *
   * The default map is found by looking up the
   *  \c vrt::collection::DefaultMap<...> specialization on the Index type.
   *
   * \param[in] range index range for the collection
   * \param[in] args arguments to pass to collection construct
   *
   * \return proxy to the new collection
   */
  template <typename ColT, typename... Args>
  CollectionProxyWrapType<ColT, typename ColT::IndexType>
  construct(typename ColT::IndexType range, Args&&... args);

  /**
   * \brief Collectively construct a new virtual context collection with
   * templated map
   *
   *  Construct virtual context collection with an explicit map. This construct
   *  method enables distributed SPMD construction of the virtual context
   *  collection where each index is mapped with the \c MapFnT.
   *
   * \param[in] range index range for the collection
   * \param[in] tag tag for out-or-order creation
   *
   * \return proxy to the new collection
   */
  template <
    typename ColT,  mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
  >
  IsDefaultConstructableType<ColT> constructCollective(
    typename ColT::IndexType range, TagType const& tag = no_tag
  );

  /**
   * \brief Collectively construct a new virtual context collection with
   * templated map
   *
   *  Construct virtual context collection with an explicit map. This construct
   *  method enables distributed SPMD construction of the virtual context
   *  collection using the \c DistribConstructFn. The system will invoke that
   *  function for every index in the system based on the where each index is
   *  mapped with the \c MapFnT.
   *
   * \param[in] range index range for the collection
   * \param[in] cons_fn construct function to create an element on each node
   * \param[in] tag tag for out-or-order creation
   *
   * \return proxy to the new collection
   */
  template <
    typename ColT,  mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
  >
  CollectionProxyWrapType<ColT> constructCollective(
    typename ColT::IndexType range, DistribConstructFn<ColT> cons_fn,
    TagType const& tag = no_tag
  );

  /**
   * \brief Collectively construct a new virtual context collection with
   * the default map.
   *
   *  Construct virtual context collection with the default map. This construct
   *  method enables distributed SPMD construction of the virtual context
   *  collection where each index is mapped with the default mapping function.
   *
   * \param[in] range index range for the collection
   * \param[in] tag tag for out-or-order creation
   *
   * \return proxy to the new collection
   */
  template <typename ColT>
  IsDefaultConstructableType<ColT> constructCollective(
    typename ColT::IndexType range, TagType const& tag = no_tag
  );

  /**
   * \brief Collectively construct a new virtual context collection with
   * the default map.
   *
   *  Construct virtual context collection with the default map. This construct
   *  method enables distributed SPMD construction of the virtual context
   *  collection using the \c DistribConstructFn. The system will invoke that
   *  function for every index in the system based on the where each index is
   *  mapped with the default mapping function for this index type selected.
   *
   * \param[in] range index range for the collection
   * \param[in] cons_fn construct function to create an element on each node
   * \param[in] tag tag for out-or-order creation
   *
   * \return proxy to the new collection
   */
  template <typename ColT>
  CollectionProxyWrapType<ColT> constructCollective(
    typename ColT::IndexType range,
    DistribConstructFn<ColT> cons_fn,
    TagType const& tag = no_tag
  );

  /**
   * \brief Collectively construct a new virtual context collection with
   * a pre-registered map function.
   *
   *  Construct virtual context collection with a pre-registered map function
   *  handler. This construct method enables distributed SPMD construction of
   *  the virtual context collection using the \c DistribConstructFn. The system
   *  will invoke that function for every index in the system based on the where
   *  each index is mapped with the registered map function.
   *
   * \param[in] range index range for the collection
   * \param[in] cons_fn construct function to create an element on each node
   * \param[in] map_han the registered map function
   * \param[in] tag tag for out-or-order creation
   *
   * \return proxy to new collection
   */
  template <typename ColT>
  CollectionProxyWrapType<ColT> constructCollectiveMap(
    typename ColT::IndexType range, DistribConstructFn<ColT> cons_fn,
    HandlerType const map_han, TagType const& tag
  );

private:
  /**
   * \internal \brief Insert into a collection on this node
   *
   * \param[in] proxy the collection proxy
   * \param[in] idx the index to insert
   * \param[in] args arguments for the collection constructor
   */
  template <typename ColT, typename... Args>
  void staticInsert(
    VirtualProxyType proxy, typename ColT::IndexType idx, Args&&... args
  );

  /**
   * \internal \brief Insert into a collection on this node with a pointer to
   * the collection element to insert
   *
   * \param[in] proxy the collection proxy
   * \param[in] idx the index to insert
   * \param[in] ptr unique ptr to insert for the collection
   */
  template <typename ColT>
  void staticInsertColPtr(
    VirtualProxyType proxy, typename ColT::IndexType idx,
    std::unique_ptr<ColT> ptr
  );

public:
  /**
   * \brief Collectively construct a virtual context collection with a staged
   * insert token to split initial construction with element insertion. Use
   * default map for index specified.
   *
   * Construct virtual context collection with user insertions before the
   * collection is used. The token must be moved into \c finishedInsert before
   * the collection can be used.
   *
   * \param[in] range index range for the collection
   * \param[in] tag tag for out of order construction
   *
   * \return insert token for performing insertions
   */
  template <typename ColT>
  InsertToken<ColT> constructInsert(
    typename ColT::IndexType range, TagType const& tag = no_tag
  );

  /**
   * \brief Collectively construct a virtual context collection with a staged
   * insert token to split initial construction with element insertion. Use
   * map specified by non-type template parameter.
   *
   * Construct virtual context collection with user insertions before the
   * collection is used. The token must be moved into \c finishedInsert before
   * the collection can be used.
   *
   * \param[in] range index range for the collection
   * \param[in] tag tag for out of order construction
   *
   * \return insert token for performing insertions
   */
  template <
    typename ColT, mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
  >
  InsertToken<ColT> constructInsert(
    typename ColT::IndexType range, TagType const& tag = no_tag
  );

  /**
   * \brief Collectively construct a virtual context collection with a staged
   * insert token to split initial construction with element insertion. Use
   * pre-registered map handler passed to method.
   *
   * Construct virtual context collection with user insertions before the
   * collection is used. The token must be moved into \c finishedInsert before
   * the collection can be used.
   *
   * \param[in] range index range for the collection
   * \param[in] map_han pre-registered map handler function
   * \param[in] tag tag for out of order construction
   *
   * \return insert token for performing insertions
   */
  template <typename ColT>
  InsertToken<ColT> constructInsertMap(
    typename ColT::IndexType range, HandlerType const map_han, TagType const& tag
  );

  /**
   * \brief Tell the system that insertions are complete on a staged insert
   * collection.
   *
   * \param[in] token insert token moved here to indicate completion of insert
   * phase
   *
   * \return the proxy for the collection
   */
  template <typename ColT>
  CollectionProxyWrapType<ColT> finishedInsert(InsertToken<ColT>&& token);

  /**
   * \brief Get the default map registered handler for a collection
   *
   * \return the registered map
   */
  template <typename ColT>
  static HandlerType getDefaultMap();

  /**
   * \brief Register a map functor for arguments
   *
   * \return the registered map
   */
  template <typename ColT, typename ParamT, typename... Args>
  static HandlerType getDefaultMapImpl(std::tuple<Args...>);

  /**
   * \internal \brief Insert meta-data for this collection on this node
   *
   * \param[in] proxy the collection proxy
   * \param[in] inner_holder_args arguments to construct the inner holder
   */
  template <typename ColT, typename... Args>
  static void insertMetaCollection(
    VirtualProxyType const& proxy, Args&&... inner_holder_args
  );

  /**
   * \internal \brief Make the next distributed proxy during collective, SPMD
   * collection construction
   *
   * \param[in] tag optional tag for out-of-order construction
   *
   * \return the collection proxy bits
   */
  template <typename=void>
  VirtualProxyType makeDistProxy(TagType const& tag = no_tag);

  /**
   * \internal \brief Construct all elements on this node during rooted
   * collection construction
   *
   * \param[in] msg holds meta-data for collection construction
   */
  template <typename SysMsgT>
  static void distConstruct(SysMsgT* msg);

  /**
   * \brief Query the current index context of the running handler
   *
   * This function can be called legally during the constructor of a virtual
   * collection element and when a handler for it is running.
   *
   * \return the running index
   */
  template <typename IndexT>
  static IndexT* queryIndexContext();

  /**
   * \brief Query the current proxy context of the running handler
   *
   * \return the running proxy
   */
  template <typename IndexT>
  static VirtualProxyType queryProxyContext();

  /**
   * \brief Check if a collection is running in the current context.
   *
   * \return whether a collection handler is running
   */
  template <typename IndexT>
  static bool hasContext();

  /**
   * \internal \brief Perform asynchronous reduction after construction of a
   * collection to determine it's fully constructed across all nodes.
   *
   * \param[in] proxy the collection proxy
   */
  template <typename ColT>
  static void reduceConstruction(VirtualProxyType const& proxy);

  /**
   * \internal \brief Construct a group for the collection
   *
   * This must be called for every collection before any reductions can safely
   * complete. If the collection spans all node, this will fire a lambda that
   * tells the system that the \c default_group can be used for broadcasts and
   * reductions. Broadcasts are always valid to run (unless the group expands
   * after LB) because it will just hit extra nodes that may not have elements.
   *
   * \param[in] proxy the collection proxy
   * \param[in] immediate whether to start group construction now
   */
  template <typename ColT>
  static void groupConstruction(VirtualProxyType const& proxy, bool immediate);

  /**
   * \internal \brief Send a message to a collection element with handler type
   * erased
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to run
   * \param[in] imm_context whether in an immediate context (running a
   * collection element handler vs. directly in the scheduler)--- if in a
   * handler, local delivery must be postponed
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    typename ColT = typename MsgT::CollectionType,
    typename IdxT = typename ColT::IndexType
  >
  messaging::PendingSend sendMsgUntypedHandler(
    VirtualElmProxyType<ColT> const& proxy, MsgT* msg,
    HandlerType const handler, bool imm_context = true
  );

  /**
   * \internal \brief Send a message to a collection element when not using a
   * collection message
   *
   * This overload will wrap the message in a CollectionMessage<ColT> to hold
   * the index meta-data for the collection send.
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to run
   */
  template <typename MsgT, typename ColT>
  IsNotColMsgType<MsgT> sendMsgWithHan(
    VirtualElmProxyType<ColT> const& proxy, MsgT* msg,
    HandlerType const handler
  );

  /**
   * \internal \brief Send a message to a collection element when using a
   * collection message
   *
   * This overload will directly send the message without promotion
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to run
   */
  template <typename MsgT, typename ColT>
  IsColMsgType<MsgT> sendMsgWithHan(
    VirtualElmProxyType<ColT> const& proxy, MsgT* msg,
    HandlerType const handler
  );

  /**
   * \internal \brief Send a normal message to a collection element with handler
   * type-erased
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to run
   *
   * \return the pending send
   */
  template <typename MsgT, typename ColT>
  messaging::PendingSend sendNormalMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT* msg,
    HandlerType const handler
  );

  /**
   * \brief Send collection element a message from active function handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <
    typename MsgT, ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
  >
  messaging::PendingSend sendMsg(
    VirtualElmProxyType<typename MsgT::CollectionType> const& proxy, MsgT *msg
  );

  /**
   * \brief Send collection element a message from active member handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
  >
  messaging::PendingSend sendMsg(
    VirtualElmProxyType<typename MsgT::CollectionType> const& proxy, MsgT *msg
  );

  /**
   * \brief Send collection element a message from active function handler with
   * a proper \c CollectionMessage<ColT>
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg
  );

  /**
   * \brief Send collection element a message from active function handler with
   * a non-collection message, doing a promotion
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsNotColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg
  );

  /**
   * \brief Send collection element a message from active member handler with a
   * proper \c CollectionMessage<ColT>
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg
  );

  /**
   * \brief Send collection element a message from active member handler with a
   * non-collection message, doing a promotion
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsNotColMsgType<MsgT> sendMsg(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg
  );

  /**
   * \internal \brief Send a collection element a message with active function
   * handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return the pending send
   */
  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  messaging::PendingSend sendMsgImpl(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg
  );

  /**
   * \internal \brief Send a collection element a message with active member
   * handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   *
   * \return the pending send
   */
  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
  >
  messaging::PendingSend sendMsgImpl(
    VirtualElmProxyType<ColT> const& proxy, MsgT *msg
  );

  /**
   * \internal \brief Deliver a message to a collection element with a promoted
   * collection message that wrapped the user's non-collection message.
   *
   * \param[in] msg the message
   * \param[in] col pointer to collection element
   * \param[in] han the handler to invoke
   * \param[in] from node that sent the message
   */
  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
  static IsWrapType<ColT, UserMsgT, MsgT> collectionMsgDeliver(
    MsgT* msg, CollectionBase<ColT, IndexT>* col, HandlerType han,
    NodeType from
  );

  /**
   * \internal \brief Deliver a message to a collection element with a normal
   * collection message
   *
   * \param[in] msg the message
   * \param[in] col pointer to collection element
   * \param[in] han the handler to invoke
   * \param[in] from node that sent the message
   */
  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
  static IsNotWrapType<ColT, UserMsgT, MsgT> collectionMsgDeliver(
    MsgT* msg, CollectionBase<ColT, IndexT>* col, HandlerType han,
    NodeType from
  );

  /**
   * \internal \brief Base collection message handler
   *
   * \param[in] msg the message
   */
  template <typename CoLT, typename IndexT>
  static void collectionMsgHandler(BaseMessage* msg);

  /**
   * \internal \brief Typed collection message handler
   *
   * \param[in] msg the message
   */
  template <typename ColT, typename IndexT, typename MsgT>
  static void collectionMsgTypedHandler(MsgT* msg);

  /**
   * \internal \brief Record statistics for collection message handler when a
   * message arrives for the element.
   *
   * Records where the message came from, size of message, and type of
   * communication (e.g., broadcast or send)
   *
   * \param[in] col_ptr the collection element pointer
   * \param[in] msg the message to deliver
   */
  template <typename ColT, typename MsgT>
  static void recordStats(ColT* col_ptr, MsgT* msg);

  /**
   * \brief Invoke function 'f' (with copyable return type) inline without going
   * through scheduler
   *
   * \param[in] proxy the collection proxy
   * \param[in] args function params
   */
  template <typename ColT, typename Type, Type f, typename... Args>
  util::Copyable<Type>
  invoke(VirtualElmProxyType<ColT> const& proxy, Args... args);

  /**
   * \brief Invoke function 'f' (with non-copyable return type) inline without
   * going through scheduler
   *
   * \param[in] proxy the collection proxy
   * \param[in] args function params
   */
  template <typename ColT, typename Type, Type f, typename... Args>
  util::NotCopyable<Type>
  invoke(VirtualElmProxyType<ColT> const& proxy, Args... args);

  /**
   * \brief Invoke function 'f' (with void return type) inline without going
   * through scheduler
   *
   * \param[in] proxy the collection proxy
   * \param[in] args function params
   */
  template <typename ColT, typename Type, Type f, typename... Args>
  util::IsVoidReturn<Type>
  invoke(VirtualElmProxyType<ColT> const& proxy, Args... args);

  /**
   * \brief Invoke message action function handler without going through scheduler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   */
  template <
    typename MsgT, ActiveColTypedFnType<MsgT, typename MsgT::CollectionType>* f
  >
  void invokeMsg(
    VirtualElmProxyType<typename MsgT::CollectionType> const& proxy,
    messaging::MsgPtrThief<MsgT> msg, bool instrument = true
  );

  /**
   * \brief Invoke message action member handler without going through scheduler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   */
  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
  >
  void invokeMsg(
    VirtualElmProxyType<typename MsgT::CollectionType> const& proxy,
    messaging::MsgPtrThief<MsgT> msg, bool instrument = true
  );

  /**
   * \internal \brief Invoke message handler without going through scheduler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message with the virtual handler
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   */
  template <typename ColT, typename MsgT>
  void invokeMsgImpl(
    VirtualElmProxyType<ColT> const& proxy, messaging::MsgPtrThief<MsgT> msg,
    bool instrument
  );

  /**
   * \brief Reduce over a collection
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the reduce message
   * \param[in] stamp the reduce stamp
   * \param[in] root_node node to receive the ultimately reduced message and run
   * the associated handler (if a callback is specified on a particular node,
   * the root will run the handler that triggers the callback at the appropriate
   * location)
   *
   * \return a PendingSend corresponding to the reduce
   */
  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  messaging::PendingSend reduceMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *const msg, ReduceStamp stamp = ReduceStamp{},
    NodeType root_node = uninitialized_destination
  );

  /**
   * \brief Reduce over a collection
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the reduce message
   * \param[in] stamp the reduce stamp
   * \param[in] idx the index of collection element being reduced
   *
   * \return a PendingSend corresponding to the reduce
   */
  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  messaging::PendingSend reduceMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *const msg, ReduceStamp stamp, typename ColT::IndexType const& idx
  );

  /**
   * \internal \brief Reduce over the whole collection or a subset of a
   * collection, based on their indices matching an expression
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the reduce message
   * \param[in] expr_fn expression function to pick indices
   * \param[in] stamp the reduce stamp
   * \param[in] root_node node to receive the ultimately reduced message and run
   * the associated handler (if a callback is specified on a particular node,
   * the root will run the handler that triggers the callback at the appropriate
   * location)
   *
   * \return a PendingSend corresponding to the reduce
   */
  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  messaging::PendingSend reduceMsgExpr(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *const msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
    ReduceStamp stamp = ReduceStamp{},
    NodeType root_node = uninitialized_destination
  );

  /**
   * \internal \brief Reduce over the whole collection or a subset of a
   * collection, based on their indices matching an expression
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the reduce message
   * \param[in] expr_fn expression function to pick indices
   * \param[in] stamp the reduce stamp
   * \param[in] idx the index of collection element being reduced
   *
   * \return a PendingSend corresponding to the reduce
   */
  template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  messaging::PendingSend reduceMsgExpr(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *const msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
    ReduceStamp stamp, typename ColT::IndexType const& idx
  );

  /**
   * \internal \brief Broadcast to collection with a promoted message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to invoke
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   */
  template <typename MsgT, typename ColT>
  IsNotColMsgType<MsgT> broadcastMsgWithHan(
    CollectionProxyWrapType<ColT> const& proxy, MsgT* msg,
    HandlerType const handler, bool instrument = true
  );

  /**
   * \internal \brief Broadcast to collection with a collection message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to invoke
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   */
  template <typename MsgT, typename ColT>
  IsColMsgType<MsgT> broadcastMsgWithHan(
    CollectionProxyWrapType<ColT> const& proxy, MsgT* msg,
    HandlerType const handler, bool instrument = true
  );

  /**
   * \internal \brief Broadcast a normal message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to invoke
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT>
  messaging::PendingSend broadcastNormalMsg(
    CollectionProxyWrapType<ColT> const& proxy, MsgT* msg,
    HandlerType const handler, bool instrument = true
  );

  /**
   * \internal \brief Broadcast a message with type-erased handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] handler the handler to invoke
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT, typename IdxT>
  messaging::PendingSend broadcastMsgUntypedHandler(
    CollectionProxyWrapType<ColT, IdxT> const& proxy, MsgT* msg,
    HandlerType const handler, bool instrument
  );

  /**
   * \brief Broadcast collective a message with action function handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
  >
  messaging::PendingSend broadcastCollectiveMsg(
    CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
    messaging::MsgPtrThief<MsgT> msg, bool instrument = true);

  /**
   * \brief Broadcast collective a message with action member handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
  >
  messaging::PendingSend broadcastCollectiveMsg(
    CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
    messaging::MsgPtrThief<MsgT> msg, bool instrument = true);

  /**
   * \internal \brief Broadcast collective a message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT>
  messaging::PendingSend broadcastCollectiveMsgImpl(
    CollectionProxyWrapType<ColT> const& proxy, MsgPtr<MsgT>& msg,
    bool instrument);

  /**
   * \brief Broadcast a message with action function handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
  >
  messaging::PendingSend broadcastMsg(
    CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \brief Broadcast a message with action member handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
  >
  messaging::PendingSend broadcastMsg(
    CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \brief Broadcast a message with action function handler with collection
   * message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \brief Broadcast a message with action function handler with normal
   * message promoted automatically to \c CollectionMessage<ColT>
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  IsNotColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \brief Broadcast a message with action member handler with collection
   * message
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \brief Broadcast a message with action member handler with normal
   * message promoted automatically to \c CollectionMessage<ColT>
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  IsNotColMsgType<MsgT> broadcastMsg(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \internal \brief Broadcast a message with active function handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
  messaging::PendingSend broadcastMsgImpl(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \internal \brief Broadcast a message with active member handler
   *
   * \param[in] proxy the collection proxy
   * \param[in] msg the message
   * \param[in] instrument whether to instrument the broadcast for load
   * balancing (some system calls use this to disable instrumentation)
   *
   * \return a pending send
   */
  template <
    typename MsgT,
    typename ColT,
    ActiveColMemberTypedFnType<MsgT,ColT> f
  >
  messaging::PendingSend broadcastMsgImpl(
    CollectionProxyWrapType<ColT> const& proxy,
    MsgT *msg, bool instrument = true
  );

  /**
   * \internal \brief Bounce broadcast off of the root node to stamp them to
   * avoid multiple delivery during migrations
   *
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <typename ColT, typename IndexT, typename MsgT>
  messaging::PendingSend broadcastFromRoot(MsgT* msg);

public:
  /*
   * Vrt collection type/handle registration for typeless dispatch
   */
  /**
   * \internal \brief Get the type-erased dispatcher handler for
   * sending/broadcasting without the collection type.
   *
   * Used for type-free sends/broadcast, like a callback that erases the type
   * but still needs to reach a typed endpoint
   *
   * \return the type-erased dispatch handler
   */
  template <typename MsgT, typename ColT>
  DispatchHandlerType getDispatchHandler();

  /**
   * \internal \brief Get pointer to the dispatcher base class in a type-erased
   * setting to operate on the collection.
   *
   * \param[in] han the registered dispatch handler
   *
   * \return the base dispatcher
   */
  template <typename=void>
  DispatchBasePtrType getDispatcher(DispatchHandlerType const& han);

private:
  /**
   * \internal \brief Buffer a broadcast for later delivery
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] epoch the bcast epoch (sequence number)
   * \param[in] msg the message
   */
  template <typename ColT, typename MsgT>
  void bufferBroadcastMsg(
    VirtualProxyType const& proxy, EpochType const& epoch, MsgT* msg
  );

  /**
   * \internal \brief Clear all buffered broadcasts for a given collection
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] epoch the bcast epoch (sequence number)
   */
  template <typename ColT>
  void clearBufferedBroadcastMsg(
    VirtualProxyType const& proxy, EpochType const& epoch
  );

  /**
   * \internal \brief Get a buffered message
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] epoch the bcast epoch (sequence number)
   *
   * \return the bcast message
   */
  template <typename ColT, typename MsgT>
  CollectionMessage<ColT>* getBufferedBroadcastMsg(
    VirtualProxyType const& proxy, EpochType const& epoch
  );

public:
  /**
   * \internal \brief Deliver a promoted/wrapped message to a collection element
   *
   * \param[in] msg the message
   * \param[in] col the collection element pointer
   * \param[in] han the handler to invoke
   * \param[in] from the node that sent it
   * \param[in] event the associated trace event
   */
  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
    static IsWrapType<ColT, UserMsgT, MsgT> collectionAutoMsgDeliver(
      MsgT* msg, CollectionBase<ColT, IndexT>* col, HandlerType han,
    NodeType from, trace::TraceEventIDType event
  );

  /**
   * \internal \brief Deliver a regular collection message to a collection
   * element
   *
   * \param[in] msg the message
   * \param[in] col the collection element pointer
   * \param[in] han the handler to invoke
   * \param[in] from the node that sent it
   * \param[in] event the associated trace event
   */
  template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
    static IsNotWrapType<ColT, UserMsgT, MsgT> collectionAutoMsgDeliver(
      MsgT* msg, CollectionBase<ColT, IndexT>* col, HandlerType han,
    NodeType from, trace::TraceEventIDType event
  );

  /**
   * \internal \brief Receive a broadcast to a collection
   *
   * \param[in] msg collection message
   */
  template <typename ColT, typename IndexT, typename MsgT>
  static void collectionBcastHandler(MsgT* msg);

  /**
   * \internal \brief Receive a broadcast at the root for stamping
   *
   * \param[in] msg the message
   */
  template <typename ColT, typename IndexT, typename MsgT>
  static void broadcastRootHandler(MsgT* msg);

  /**
   * \internal \brief Inform that collection has finished construction
   *
   * \param[in] msg the message
   */
  template <typename=void>
  static void collectionFinishedHan(CollectionConsMsg* msg);

  /**
   * \internal \brief Reduction target after group construction
   *
   * \param[in] msg the message
   */
  template <typename=void>
  static void collectionGroupReduceHan(CollectionGroupMsg* msg);

  /**
   * \internal \brief Inform group finished construction
   *
   * \param[in] msg the message
   */
  template <typename=void>
  static void collectionGroupFinishedHan(CollectionGroupMsg* msg);

  /**
   * \internal \brief Count the number of elements for a collection on this node
   *
   * Used for automatic group creation for each collection instance for
   *  broadcasts (optimization) and reduce (correctness).
   *
   * \param[in] proxy the collection proxy bits
   *
   * \return number of local elmeents
   */
  template <typename ColT, typename IndexT>
  std::size_t groupElementCount(VirtualProxyType const& proxy);

  /**
   * \internal \brief Create the group
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] in_group whether this node is in the group
   *
   * \return the new group ID
   */
  template <typename ColT, typename IndexT>
  GroupType createGroupCollection(
    VirtualProxyType const& proxy, bool const in_group
  );

  /**
   * \internal \brief Run the collection element's constructor
   *
   * This is the non-traits version of running the constructor: does not require
   * the detection idiom to dispatch to constructor.
   *
   * The alternative traits version of running the constructor based on the
   * detected available constructor types will be accessed through the traits
   * class directly by invoking the constructor. The traits-based alternative
   * is \c DerefCons::derefTuple
   *
   * \param[in] elms number of elements
   * \param[in] idx the index for this element
   * \param[in] tup the constructor args in a tuple
   *
   * \return unique pointer to the new element
   */
  template <typename ColT, typename IndexT, typename Tuple, size_t... I>
  static VirtualPtrType<ColT, IndexT> runConstructor(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  );

  /**
   * \internal \brief Insert a new collection element
   *
   * System call to insert a new collection element on this node. Called either
   * during construction/staged insertion or when an element migrates into this
   * node. Before this is called, one must construct the element. Then, one must
   * use the \c CollectionTypeAttorney to do the setup for the element---which
   * sets the index and other properties on the element. Once that is done, this
   * call will register the element with the location manager and add it to the
   * collection holders.
   *
   * \param[in] vc unique pointer to the new, constructed collection element
   * \param[in] idx element index
   * \param[in] max_idx index range for collection
   * \param[in] map_han registered map handler
   * \param[in] proxy collection proxy bits
   * \param[in] is_static whether the collection is statically sized
   * \param[in] home_node the home node for this element
   * \param[in] is_migrated_in whether it just migrated in
   * \param[in] migrated_from where it migrated in from
   *
   * \return whether it successfully inserted the element
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  bool insertCollectionElement(
    VirtualPtrType<ColT, IndexT> vc, IndexT const& idx, IndexT const& max_idx,
    HandlerType const map_han, VirtualProxyType const& proxy,
    bool const is_static, NodeType const& home_node,
    bool const& is_migrated_in = false,
    NodeType const& migrated_from = uninitialized_destination
  );

private:

  /**
   * \internal \brief Get the collection element pointer for given proxy
   *
   * \param[in] proxy the collection proxy
   *
   * \return the collection element pointer
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  ColT* getCollectionPtrForInvoke(VirtualElmProxyType<ColT> const& proxy);

  /**
   * \internal \brief Get the entire collection system holder
   *
   * \param[in] proxy the collection proxy bits
   *
   * \return the collection holder
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  CollectionHolder<ColT, IndexT>* findColHolder(VirtualProxyType const& proxy);

  /**
   * \internal \brief Get the collection element holder
   *
   * \param[in] proxy the collection proxy bits
   *
   * \return the element collection holder
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  Holder<ColT, IndexT>* findElmHolder(VirtualProxyType const& proxy);

  /**
   * \internal \brief Get the collection element holder
   *
   * \param[in] proxy the collection proxy
   *
   * \return the element collection holder
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  Holder<ColT, IndexT>* findElmHolder(CollectionProxyWrapType<ColT> proxy);

public:
  /**
   * \brief Destroy a collection
   *
   * \param[in] proxy the collection proxy
   */
  template <typename ColT, typename IndexT>
  void destroy(CollectionProxyWrapType<ColT,IndexT> const& proxy);

private:
  /**
   * \internal \brief Incoming message to run all cleanup functions
   *
   * \param[in] proxy the collection proxy
   */
  template <typename ColT, typename IndexT>
  void incomingDestroy(CollectionProxyWrapType<ColT,IndexT> const& proxy);

  /**
   * \internal \brief Destroy all elements and related meta-data for a proxy
   * across all system data structures
   *
   * \param[in] proxy the collection proxy
   */
  template <typename ColT, typename IndexT>
  void destroyMatching(CollectionProxyWrapType<ColT,IndexT> const& proxy);

protected:
  /**
   * \internal \brief Make the next collection proxy bits
   *
   * \return the new proxy
   */
  VirtualProxyType makeNewCollectionProxy();

  /**
   * \internal \brief Insert collection into \c UniversalIndexHolder
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] map the map function
   * \param[in] insert_epoch insert epoch for dynamic insertions
   */
  void insertCollectionInfo(
    VirtualProxyType const& proxy, HandlerType const map,
    EpochType const& insert_epoch = no_epoch
  );

private:
  /**
   * \internal \brief Schedule an action for later
   *
   * \param[in] action the action to schedule
   */
  void schedule(ActionType action);

  /**
   * \internal \brief Schedule an action with an associated message for delivery
   * later unless \c execute_now is set; in that case, run it right away.
   *
   * \param[in] msg the message
   * \param[in] execute_now whether to execute now
   * \param[in] cur_epoch the message epoch
   * \param[in] action associated action
   *
   * \return the pending send
   */
  template <typename MsgT>
  messaging::PendingSend schedule(
    MsgT msg, bool execute_now, EpochType cur_epoch, ActionType action
  );

public:
  /**
   * \brief Get the default mapped node for an element
   *
   * \param[in] proxy the collection proxy
   * \param[in] idx the index
   *
   * \return the mapped node
   */
  template <typename ColT, typename IndexT>
  NodeType getMappedNode(
    CollectionProxyWrapType<ColT,IndexT> const& proxy,
    typename ColT::IndexType const& idx
  );

  /**
   * \brief Migrate element to a new node
   *
   * Element must exist on this node to call this method.
   *
   * \param[in] proxy the element proxy
   * \param[in] dest the destination node
   *
   * \return status of migration operation
   */
  template <typename ColT>
  MigrateStatus migrate(
    VrtElmProxy<ColT, typename ColT::IndexType> proxy, NodeType const& dest
  );

  /**
   * \internal \brief Handler to insert an element on this node
   *
   * \param[in] msg insert message
   */
  template <typename ColT, typename IndexT>
  static void insertHandler(InsertMsg<ColT,IndexT>* msg);

  /**
   * \internal \brief Handler to query home before inserting on this node
   *
   * \param[in] msg insert message
   */
  template <typename ColT, typename IndexT>
  static void pingHomeHandler(InsertMsg<ColT,IndexT>* msg);

  /**
   * \internal \brief Dynamically insert an element or send a message to mapped
   * node to insert. If the \c node parameter is set, ignore the mapped node and
   * insert wherever specified by the user
   *
   * \param[in] proxy the collection proxy
   * \param[in] idx the index to insert
   * \param[in] node the node to insert on
   * \param[in] pinged_home_already whether the home node has been contacted to
   * ensure an insertion has not occurred already
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  void insert(
    CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx,
    NodeType const& node = uninitialized_destination,
    bool pinged_home_already = false
  );

  /**
   * \brief Try to get a pointer to a collection element
   *
   * \warning Migration may invalidate this pointer. It is not recommended to
   * hold the pointer obtained.
   *
   * \param[in] proxy the collection proxy
   * \param[in] idx the index
   *
   * \return raw pointer to element
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  ColT* tryGetLocalPtr(
    CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx
  );

  /**
   * \internal \brief Done with insertions
   *
   * \param[in] msg done insert message
   */
  template <typename ColT, typename IndexT>
  static void doneInsertHandler(DoneInsertMsg<ColT,IndexT>* msg);

  /**
   * \internal \brief Do a dynamic insertion handler
   *
   * \param[in] msg insert message
   */
  template <typename ColT, typename IndexT>
  static void actInsertHandler(ActInsertMsg<ColT,IndexT>* msg);

  /**
   * \internal \brief Update the insert epoch
   *
   * \param[in] msg the update insert message
   */
  template <typename ColT, typename IndexT>
  static void updateInsertEpochHandler(UpdateInsertMsg<ColT,IndexT>* msg);

  /**
   * \internal \brief Finished insert
   *
   * \param[in] msg the finished update
   */
  template <typename=void>
  static void finishedUpdateHan(FinishedUpdateMsg* msg);

  /**
   * \internal \brief Trigger insert actions after insert epoch terminates
   *
   * \param[in] proxy the collection proxy bits
   */
  template <typename=void>
  void actInsert(VirtualProxyType const& proxy);

  /**
   * \internal \brief Setup the next insertion epoch
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] insert_epoch the insert epoch
   */
  template <typename ColT, typename IndexT>
  void setupNextInsertTrigger(
    VirtualProxyType const& proxy, EpochType const& insert_epoch
  );

  /**
   * \brief Tell VT that insertions are complete
   *
   * \param[in] proxy the collection proxy
   * \param[in] insert_action action to execute after insertions complete
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  void finishedInserting(
    CollectionProxyWrapType<ColT,IndexT> const& proxy,
    ActionType insert_action = nullptr
  );

  /**
   * \internal \brief Add a cleanup function for a collection after destruction
   *
   * \param[in] proxy the collection proxy bits
   */
  template <typename ColT>
  void addCleanupFn(VirtualProxyType proxy);

private:
  /**
   * \internal \brief Finish insertion epoch
   *
   * \param[in] proxy the collection proxy
   * \param[in] insert_epoch insert epoch
   */
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

  template <typename ColT, typename IndexT>
  friend struct InsertTokenRval;

  friend struct balance::ElementStats;

  /**
   * \internal \brief Migrate an element out of this node
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] idx the index
   * \param[in] dest the destination node
   *
   * \return migration status
   */
  template <typename ColT, typename IndexT>
  MigrateStatus migrateOut(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& dest
  );

  /**
   * \internal \brief Migrate an element into this node
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] idx the index
   * \param[in] from node it migrated out of
   * \param[in] vrt_elm_ptr unique pointer to the element
   * \param[in] range index range for collection
   * \param[in] map_han registered map handler
   *
   * \return migration status
   */
  template <typename ColT, typename IndexT>
  MigrateStatus migrateIn(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& from,
    VirtualPtrType<ColT, IndexT> vrt_elm_ptr, IndexT const& range,
    HandlerType const map_han
  );

public:
  /**
   * \brief Register listener function for a given collection
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] fn the listener function
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  int registerElementListener(
    VirtualProxyType proxy, listener::ListenFnType<IndexT> fn
  );

  /**
   * \brief Unregister listener function for a given collection
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] element the index of the registered listener function
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  void unregisterElementListener(VirtualProxyType proxy, int element);

  /**
   * \brief Get the range that a collection was constructed with
   *
   * \param[in] proxy the proxy of the collection
   *
   * \return the range of the collection
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  IndexT getRange(VirtualProxyType proxy);

  /**
   * \brief Make the filename for checkpoint/restore
   *
   * \param[in] range range for collection
   * \param[in] idx index of element
   * \param[in] file_base base file name
   * \param[in] make_sub_dirs whether to make sub-directories for elements:
   * useful when the number of collection elements are large
   * \param[in] files_per_directory number of files to output for each sub-dir
   *
   * \return full path of a file for the element
   */
  template <typename IndexT>
  std::string makeFilename(
    IndexT range, IndexT idx, std::string file_base,
    bool make_sub_dirs, int files_per_directory
  );

  /**
   * \brief Make the filename for meta-data related to checkpoint/restore
   *
   * \param[in] file_base base file name
   *
   * \return meta-data file name for the node
   */
  template <typename IndexT>
  std::string makeMetaFilename(std::string file_base, bool make_sub_dirs);

  /**
   * \brief Checkpoint the collection (collective). Must wait for termination
   * (consistent snapshot) of work on the collection before invoking.
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] file_base the base file name of the files write
   * \param[in] make_sub_dirs whether to make sub-directories for elements:
   * useful when the number of collection elements are large
   * \param[in] files_per_directory number of files to output for each sub-dir
   *
   * \return the range of the collection
   */
  template <typename ColT, typename IndexT = typename ColT::IndexType>
  void checkpointToFile(
    CollectionProxyWrapType<ColT> proxy, std::string const& file_base,
    bool make_sub_dirs = true, int files_per_directory = 4
  );

  /**
   * \brief Restore the collection (collective) from file.
   *
   * \note Resets the phase to 0 for every element.
   *
   * \param[in] range the range of the collection to restart
   * \param[in] file_base the base file name for the files to read
   *
   * \return proxy to the new collection
   */
  template <typename ColT>
  CollectionProxyWrapType<ColT> restoreFromFile(
    typename ColT::IndexType range, std::string const& file_base
  );

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | buffers_
      | proxy_state_
      | cleanup_fns_
      | constructed_
      | reduce_cur_stamp_
      | insert_finished_action_
      | user_insert_action_
      | dist_tag_id_
      | release_lb_
      | cur_context_temp_elm_id_
      | cur_context_perm_elm_id_
      | collect_stats_for_lb_;
  }

private:
  /**
   * \brief Next proxy identifier for assignment of virtual proxy IDs
   */
  template <typename=void>
  static VirtualIDType curIdent_;

  /**
   * \brief Buffered broadcasts
   */
  template <typename ColT>
  static BcastBufferType<ColT> broadcasts_;

  /**
   * \internal \brief Get the current LB temporary element ID based on handler
   * context
   *
   * \return the element ID
   */
  balance::ElementIDType getCurrentContextTemp() const;

  /**
   * \internal \brief Get the current LB permanent element ID based on handler
   * context
   *
   * \return the element ID
   */
  balance::ElementIDType getCurrentContextPerm() const;

  /**
   * \internal \brief Set the current LB element ID
   *
   * \param[in] elm_perm permanent ID
   * \param[in] elm_temp temporary ID
   */
  void setCurrentContext(
    balance::ElementIDType elm_perm, balance::ElementIDType elm_temp
  );

  /**
   * \internal \brief Get the mapped node for an element
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] cur the index
   *
   * \return the mapped node
   */
  template <typename ColT, typename IdxT = typename ColT::IndexType>
  NodeType getMapped(VirtualProxyType proxy, IdxT cur) {
    auto map_han = UniversalIndexHolder<>::getMap(proxy);
    auto max = getRange<ColT>(proxy);
    return getMapped<ColT>(map_han, cur, max);
  }

  /**
   * \internal \brief Get the mapped node for an element with a specific map
   * function
   *
   * \param[in] proxy the collection proxy bits
   * \param[in] map_han the registered map
   * \param[in] cur the index
   *
   * \return the mapped node
   */
  template <typename ColT, typename IdxT = typename ColT::IndexType>
  NodeType getMapped(VirtualProxyType proxy, HandlerType map_han, IdxT cur) {
    auto max = getRange<ColT>(proxy);
    return getMapped<ColT>(map_han, cur, max);
  }

  /**
   * \internal \brief Get the mapped node for an element with a specific map
   * function and the index range
   *
   * \param[in] map_han the registered map
   * \param[in] cur the index
   * \param[in] max the index range for the collection
   *
   * \return the mapped node
   */
  template <typename ColT, typename IdxT = typename ColT::IndexType>
  NodeType getMapped(HandlerType map_han, IdxT cur, IdxT max) {
    auto fn = auto_registry::getHandlerMap(map_han);
    auto const cur_base = static_cast<vt::index::BaseIndex*>(&cur);
    auto const max_base = static_cast<vt::index::BaseIndex*>(&max);
    return fn(cur_base, max_base, theContext()->getNumNodes());
  }

  using ActionPendingType = std::function<messaging::PendingSend(void)>;

  /**
   * \brief Add to the current buffer-release state of a proxy
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] release the state to | into the bits
   */
  void addToState(VirtualProxyType proxy, BufferReleaseEnum release) {
    proxy_state_[proxy] =
      static_cast<BufferReleaseEnum>(proxy_state_[proxy] | release);
  }

  /**
   * \brief Get the current release state bits from the collection
   *
   * \param[in] proxy the proxy of the collection
   *
   * \return the current state
   */
  BufferReleaseEnum getState(VirtualProxyType proxy) {
    auto iter = proxy_state_.find(proxy);
    return iter != proxy_state_.end() ? iter->second : BufferReleaseEnum::Unreleased;
  }

  /**
   * \brief Get ready bits AND'ed with the required release bits
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] release the bits to check
   *
   * \return the current state & release
   */
  BufferReleaseEnum getReadyBits(
    VirtualProxyType proxy, BufferReleaseEnum release
  );

  /**
   * \brief Check if the collection is past the stages of setup requested in
   * release
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] release the bits to check
   *
   * \return whether it is ready
   */
  bool checkReady(VirtualProxyType proxy, BufferReleaseEnum release);

  /**
   * \brief Buffer an operation until release bits are true
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] type the type of operation
   * \param[in] release the release state required to execute this operation
   * \param[in] epoch the epoch associated with the operation
   * \param[in] action the action to execute when ready
   *
   * \return a pending send
   */
  template <typename ColT>
  messaging::PendingSend bufferOp(
    VirtualProxyType proxy, BufferTypeEnum type, BufferReleaseEnum release,
    EpochType epoch, ActionPendingType action
  );

  /**
   * \brief Buffer an operation until release bits are true. Execute it if the
   * release bits are set properly immediately.
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] type the type of operation
   * \param[in] release the release state required to execute this operation
   * \param[in] epoch the epoch associated with the operation
   * \param[in] action the action to execute when ready
   *
   * \return a pending send
   */
  template <typename ColT>
  messaging::PendingSend bufferOpOrExecute(
    VirtualProxyType proxy, BufferTypeEnum type, BufferReleaseEnum release,
    EpochType epoch, ActionPendingType action
  );

  /**
   * \brief Trigger ready operation for a certain type
   *
   * \param[in] proxy the proxy of the collection
   * \param[in] type the type of operation to release
   */
  void triggerReadyOps(VirtualProxyType proxy, BufferTypeEnum type);

  using ActionPendingVecType = std::vector<ActionPendingType>;
  using ReleaseToAction = std::unordered_map<BufferReleaseEnum, ActionPendingVecType>;
  using BufferToRelease = std::unordered_map<BufferTypeEnum, ReleaseToAction>;
  using ProxyToBuffer = std::unordered_map<VirtualProxyType, BufferToRelease>;
  using ProxyToState = std::unordered_map<VirtualProxyType, BufferReleaseEnum>;

  ProxyToBuffer buffers_;
  ProxyToState proxy_state_;

  CleanupListFnType cleanup_fns_;
  std::unordered_set<VirtualProxyType> constructed_;
  std::unordered_map<ReduceVirtualIDType,ReduceStamp> reduce_cur_stamp_;
  std::unordered_map<VirtualProxyType,ActionType> collect_stats_for_lb_;
  std::unordered_map<VirtualProxyType,ActionVecType> insert_finished_action_ = {};
  std::unordered_map<VirtualProxyType,ActionVecType> user_insert_action_ = {};
  std::unordered_map<TagType,VirtualIDType> dist_tag_id_ = {};
  std::unordered_map<VirtualProxyType,ActionType> release_lb_ = {};
  balance::ElementIDType cur_context_temp_elm_id_ = balance::no_element_id;
  balance::ElementIDType cur_context_perm_elm_id_ = balance::no_element_id;
};

// These are static variables in class templates because Intel 18
// dislikes static class member variable templates
namespace details
{
  template <typename T>
  struct CurIdent
  {
    static VirtualIDType m_;
  };

  template <typename ColT>
  struct Broadcasts
  {
    static typename CollectionManager::BcastBufferType<ColT> m_;
  };
}

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/manager.impl.h"
#include "vt/vrt/collection/migrate/manager_migrate_attorney.impl.h"
#include "vt/vrt/collection/send/sendable.impl.h"
#include "vt/vrt/collection/gettable/gettable.impl.h"
#include "vt/vrt/collection/reducable/reducable.impl.h"
#include "vt/vrt/collection/invoke/invokable.impl.h"
#include "vt/vrt/collection/insert/insertable.impl.h"
#include "vt/vrt/collection/insert/insert_finished.impl.h"
#include "vt/vrt/collection/destroy/destroyable.impl.h"
#include "vt/vrt/collection/destroy/manager_destroy_attorney.impl.h"
#include "vt/vrt/collection/broadcast/broadcastable.impl.h"
#include "vt/vrt/collection/rdmaable/rdmaable.impl.h"
#include "vt/vrt/collection/balance/elm_stats.impl.h"
#include "vt/vrt/collection/types/insertable.impl.h"
#include "vt/vrt/collection/types/indexable.impl.h"
#include "vt/vrt/collection/dispatch/dispatch.impl.h"
#include "vt/vrt/collection/dispatch/registry.impl.h"
#include "vt/vrt/collection/staged_token/token.impl.h"
#include "vt/vrt/collection/types/base.impl.h"
#include "vt/rdmahandle/manager.collection.impl.h"
#include "vt/vrt/proxy/collection_proxy.impl.h"

#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast.impl.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_H*/
