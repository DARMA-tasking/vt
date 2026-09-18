/*
//@HEADER
// *****************************************************************************
//
//                                  location.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_LOCATION_H
#define INCLUDED_VT_TOPOS_LOCATION_LOCATION_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.fwd.h"
#include "vt/topos/location/utility/entity.h"
#include "vt/topos/location/message/msg.h"
#include "vt/context/context.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/vrt_common.h"
#include "vt/objgroup/manager.h"

#include <loc/coordinator.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <type_traits>
#include <utility>

namespace vt { namespace location {

struct collection_lm_tag_t {};

template <typename EntityID>
struct EntityLocationCoord;

/**
 * \brief Adapter satisfying loc::Communicator with VT objgroup messaging.
 *
 * The containing EntityLocationCoord is already a collectively-created
 * objgroup, so its proxy is also the collective instance handle expected by
 * loc. Control-plane RPCs are dispatched to the embedded loc coordinator.
 */
template <typename EntityID>
struct LocCommunicator {
  using OwnerType = EntityLocationCoord<EntityID>;

  template <typename Instance>
  using HandleType = objgroup::proxy::Proxy<OwnerType>;

  explicit LocCommunicator(OwnerType* in_owner) : owner_(in_owner) { }

  int getRank() const;

  template <typename Instance>
  HandleType<Instance> registerInstanceCollective(Instance* instance);

  template <auto Handler, typename... Args>
  void send(
    ::loc::NodeType node, objgroup::proxy::Proxy<OwnerType> handle,
    Args&&... args
  );

private:
  OwnerType* owner_ = nullptr;
};

/**
 * \struct EntityLocationCoord
 *
 * \brief Part of a core VT component that manages the distributed location of
 * virtual entities.
 *
 * Allows general registration of an \c EntityID that is tracked across the
 * system as it migrates. DARMA/loc owns the distributed directory, resolution,
 * and cache-coherence protocol through \c ResolverType. VT retains the entity
 * message delivery, handler dispatch, and termination-epoch integration layered
 * on top of the resolved node.
 *
 * The \c registerEntity method allows an external component to locally register
 * an entity as existing on this node. If the entity is deleted, \c
 * unregisterEntity should be called; if the entity is migrated, \c
 * entityEmigrated should be invoked on the node from which the entity is
 * emigrating. A message may arrive for the entity by a location coordinator
 * calling \c routeMsg to the associated entity.
 *
 */
template <typename EntityID>
struct EntityLocationCoord {
  using ThisType = EntityLocationCoord<EntityID>;
  using ResolverCommType = LocCommunicator<EntityID>;
  using ResolverType = ::loc::Coordinator<EntityID, ResolverCommType>;
  using LocEntityMsg = LocEntity<EntityID>;
  using LocalRegisteredContType = std::unordered_set<EntityID>;
  using LocalRegisteredMsgContType = std::unordered_map<EntityID, LocEntityMsg>;
  using ActionListType = std::vector<NodeActionType>;
  using PendingLocLookupsType = std::unordered_map<EntityID, ActionListType>;

  template <typename MessageT>
  using EntityMsgType = EntityMsg<EntityID, MessageT>;

  /**
   * \brief Construct a new location manager with defaults
   */
  EntityLocationCoord() : resolver_comm_(this) { }

  /**
   * \brief Construct with parameters
   *
   * \param[in] in_anytime_migration whether anytime migration is allowed
   * \param[in] in_keep_cache_updated whether to keep the cache updated
   * \param[in] in_max_cache_size what the max cache size is
   */
  explicit EntityLocationCoord(
    bool in_anytime_migration, bool in_keep_cache_updated,
    std::size_t in_max_cache_size = ::loc::default_max_cache_size
  ) : resolver_comm_(this),
      anytime_migration_(in_anytime_migration),
      keep_cache_updated_(in_keep_cache_updated),
      max_cache_size_(in_max_cache_size)
  { }


  virtual ~EntityLocationCoord() {}

  /**
   * \brief Register a new entity
   *
   * \param[in] id the entity ID
   * \param[in] home the home node for this entity
   * \param[in] msg_action function to trigger when message arrives for it
   * \param[in] migrated whether it migrated in: \c entityEmigrated is preferred
   */
  void registerEntity(
    EntityID const& id, NodeType const& home,
    LocMsgActionType msg_action = nullptr, bool const& migrated = false
  );

  /**
   * \brief Register a new entity that has been created remotely---on another
   * rank off the home node.
   *
   * \note Example: an insertion occurs off home node. The insertion node sends
   * a message to the home and makes this invocation
   *
   * \param[in] id the entity ID
   * \param[in] home the home node for this entity
   * \param[in] create_node the node where the creation is occurring
   * \param[in] msg_action function to trigger when message arrives for it
   */
  void registerEntityRemote(
    EntityID const& id, NodeType const& home, NodeType const create_node,
    LocMsgActionType msg_action = nullptr
  );

  /**
   * \brief Unregister an entity
   *
   * \param[in] id the entity ID
   */
  void unregisterEntity(EntityID const& id);

  /**
   * \brief Tell the location manager that migrations are going to start
   *
   * \note Must be used when anytime migration is off
   */
  void startMigrations();

  /**
   * \brief Indicate that migrations are complete
   *
   * \note Must be used when anytime migration is off
   */
  void doneMigrations();

  /**
   * \brief Tell coordinator that the entity has migrated to another node
   *
   * \param[in] id the entity ID
   * \param[in] new_node the node it was migrated to
   */
  void entityEmigrated(EntityID const& id, NodeType const& new_node);

  /**
   * \brief Register a migrated entity on new node
   *
   *
   * This should be called after the entity is migrated when it arrived on the
   * new node: order of operations:
   *
   *   1) Node 0: registerEntity(my_id, ...);
   *   2) Node 0: entityEmigrated(my_id, 1);
   *   3) Node 1: entityImmigrated(my_id, <home>, 0, ...);
   *
   * \param[in] id the entity ID
   * \param[in] home_node the home node for the entity
   * \param[in] msg_action function to trigger when message arrives for it
   */
  void entityImmigrated(
    EntityID const& id, NodeType const& home_node,
    NodeType const& __attribute__((unused)) from_node,
    LocMsgActionType msg_action = nullptr
  );

  /**
   * \brief Get the location of an entity
   *
   * Get the location of a entity: the `action' is triggered when the location
   * of the entity is resolved. This is an asynchronous call that may send
   * messages to discover the location of the entity `id'. To resolve the
   * location the method uses the following algorithm:
   *
   *   1) Check locally for the entity's existence
   *   2) If not local, search for a cache entry with location info
   *   3) If no cache information available, send resolution message to home node.
   *     a) The home node applies the same algorithm, starting with (1)
   *     b) On step 3, if no information is known, the manager buffers the
   *        request, waiting to the entity to be registered in the future.
   *
   * \note Migrations may make this information inaccurate; the node delivered
   * to `action' reflects the current known state, which may be remote.
   *
   * \param[in] id the entity ID
   * \param[in] home_node the home node for the entity
   * \param[in] action the action to trigger with the discovered location
   */
  void getLocation(
    EntityID const& id, NodeType const& home_node, NodeActionType const& action
  );

  /**
   * \brief Check if the entity exists locally on this rank
   *
   * \param[in] id the entity ID
   *
   * \return whether it exists locally
   */
  bool entityExistsLocal(EntityID const& id) const;

  /**
   * \brief Check if an entity exists in the system
   *
   * \param[in] id the entity ID
   * \param[in] home_node the home node for \c id
   * \param[in] action the exists/node action
   */
  void entityExists(
    EntityID const& id, NodeType const& home_node,
    ExistsNodeActionType const& action
  );

  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void setupMessageForRouting(
    EntityID const& id, NodeType const& home_node,
    MsgSharedPtr<MessageT> const& msg
  );

  /**
   * \brief Route a message with a custom handler
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for entity
   * \param[in] m message shared pointer
   */
  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void routeMsgHandler(
    EntityID const& id, NodeType const& home_node,
    MsgSharedPtr<MessageT> const& msg
  );

  /**
   * \brief Route a message with a custom handler
   *
   * \param[in] m message shared pointer
   */
  template <typename MessageT>
  void routePreparedMsgHandler(MsgSharedPtr<MessageT> const& msg);

  /**
   * \brief Route a message with a custom handler
   *
   * \param[in] m message shared pointer
   */
  template <typename MessageT>
  void routePreparedMsg(MsgSharedPtr<MessageT> const& msg);

  /**
   * \brief Route a message with a custom handler where the element is local
   *
   * \param[in] m message shared pointer
   */
  template <typename MessageT>
  void routeMsgHandlerLocal(MsgSharedPtr<MessageT> const& msg);

  /**
   * \brief Route a message to the default handler
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for the entity
   * \param[in] msg pointer to the message
   * \param[in] from_node the sending node (optional)
   */
  template <typename MessageT>
  void routeMsg(
    EntityID const& id, NodeType const& home_node,
    MsgSharedPtr<MessageT> const& msg,
    NodeType from_node = uninitialized_destination
  );

  /**
   * \internal \brief Route a message with non-eager protocol
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for the entity
   * \param[in] action action once entity is found
   */
  void routeNonEagerAction(
    EntityID const& id, NodeType const& home_node, ActionNodeType action
  );

  /**
   * \internal \brief Output the current cache state
   */
  void printCurrentCache() const;

  /**
   * \internal \brief Check if the purported location of an entity is cached
   *
   * \param[in] id the entity ID
   *
   * \return whether it is cached
   */
  bool isCached(EntityID const& id) const;

  /**
   * \internal \brief Clear the cache
   */
  void clearCache();

  /**
   * \internal \brief Classify the legacy eager/rendezvous message size
   *
   * Kept for source compatibility and diagnostics. The loc-backed coordinator
   * resolves both sizes before VT delivers them; the threshold remains \c
   * small_msg_max_size.
   *
   * \param[in] msg the message to check
   *
   * \return whether it is of eager size
   */
  template <typename MessageT>
  bool useEagerProtocol(MsgSharedPtr<MessageT> const& msg) const;

  /**
   * \brief Set the proxy for the objgroup
   *
   * \param[in] proxy proxy to set
   */
  void setProxy(objgroup::proxy::Proxy<EntityLocationCoord<EntityID>> proxy) {
    proxy_ = proxy;
    resolver_ = std::make_unique<ResolverType>(resolver_comm_, max_cache_size_);
  }

  template <auto Handler, typename... Args>
  void locControlHandler(Args... args) {
    vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
    std::invoke(Handler, resolver_.get(), std::move(args)...);
  }

  /**
   * \brief Get local entities
   *
   * \return vector of local entities
   */
  std::vector<EntityID> getLocalEntities() const;

  /**
   * \brief All-reduce the global map of entity location
   *
   * \warning This is not scalable and will centralize all the data
   *
   * \return the global map of locations
   */
  std::unordered_map<EntityID, NodeType> buildGlobalMap();

private:
  friend struct LocCommunicator<EntityID>;

  /**
   * \brief \internal The global map handler for the all-reduce to collect up
   * entity location
   *
   *  \param[in] global_map the global map reduced
   */
  void globalMapHandler(
    std::unordered_map<EntityID, NodeType> const& global_map
  );

  /**
   * \internal \brief Handle relocation on different node.
   *
   * \param[in] msg the message
   */
  template <typename MessageT>
  void routedHandler(MessageT *msg);

  /**
   * \internal \brief Route a message to destination with rendezvous protocol
   *
   * \param[in] id the entity ID
   * \param[in] home_node the home node
   * \param[in] to_node destination node
   * \param[in] msg the message to route
   */
  template <typename MessageT>
  void routeMsgNode(
    EntityID const& id, NodeType const& home_node, NodeType const& to_node,
    MsgSharedPtr<MessageT> const& msg
  );

  /**
   * \internal \brief Insert a pending entity action
   *
   * Add actions that are waiting on an entity to be registered on this
   * node. Once \c registerEntity is called, these actions will get triggered.
   *
   * \param[in] id the entity ID
   * \param[in] action action to execute
   */
  void insertPendingEntityAction(EntityID const& id, NodeActionType action);

private:
  /// message handlers for local registrations
  LocalRegisteredMsgContType local_registered_msg_han_;

  /// registered entities
  LocalRegisteredContType local_registered_;

  /// routed deliveries waiting for local entity registration
  PendingLocLookupsType pending_lookups_;

  /// the location manager's objgroup proxy
  objgroup::proxy::Proxy<EntityLocationCoord<EntityID>> proxy_;

  /// VT transport adapter and the backend-neutral resolver from DARMA/loc
  ResolverCommType resolver_comm_;
  std::unique_ptr<ResolverType> resolver_;

  /// Whether anytime migration can happen for this LM
  bool anytime_migration_ = true;

  /// Whether migrations are allowed (required when anytime migration is off)
  bool migrations_ongoing_ = false;

  /// Waiting for global map handler to finish
  bool waiting_global_map_handler_ = false;

  /// Whether to keep the cache up-to-date at all times
  bool keep_cache_updated_ = false;

  /// Maximum number of non-home records cached by loc
  std::size_t max_cache_size_ = ::loc::default_max_cache_size;

  /// Temporary storage for global map while reducing
  std::unordered_map<EntityID, NodeType> global_map_temp_;
};

}}  // end namespace vt::location

#include "vt/topos/location/location.impl.h"

#endif /*INCLUDED_VT_TOPOS_LOCATION_LOCATION_H*/
