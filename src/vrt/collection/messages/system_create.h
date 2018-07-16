
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/types/headers.h"
#include "messaging/message.h"
#include "serialization/serialization.h"
#include "collective/reduce/reduce.h"
#include "vrt/proxy/collection_wrapper.h"

namespace vt { namespace vrt { namespace collection {

template <
  typename RemoteInfo, typename ArgsTuple, typename CollectionT, typename IndexT
>
struct CollectionCreateMsg : ::vt::Message {
  using CollectionType = CollectionT;
  using IndexType = IndexT;
  using ArgsTupleType = ArgsTuple;

  RemoteInfo info;
  ArgsTuple tup;
  HandlerType map;

  CollectionCreateMsg() = default;
  CollectionCreateMsg(
    HandlerType const& in_han, ArgsTuple&& in_tup
  ) : ::vt::Message(), tup(std::forward<ArgsTuple>(in_tup)), map(in_han)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | info | tup | map;
  }
};

struct CollectionConsMsg : ::vt::collective::reduce::ReduceMsg {
  CollectionConsMsg() = default;
  explicit CollectionConsMsg(VirtualProxyType const& in_proxy)
    : proxy(in_proxy)
  { }

  VirtualProxyType getProxy() const { return proxy; }

  VirtualProxyType proxy = {};
};

struct CollectionGroupMsg : CollectionConsMsg {
  CollectionGroupMsg() = default;
  CollectionGroupMsg(
    VirtualProxyType const& in_proxy, GroupType const& in_group
  ) : CollectionConsMsg(in_proxy), group_(in_group)
  { }

  GroupType getGroup() const { return group_; }

private:
  GroupType group_ = no_group;
};

struct FinishedUpdateMsg : ::vt::collective::reduce::ReduceMsg {
  FinishedUpdateMsg() = default;
  explicit FinishedUpdateMsg(VirtualProxyType const& in_proxy)
    : proxy_(in_proxy)
  { }

  VirtualProxyType proxy_ = {};
};

struct CollectionPhaseMsg : ::vt::Message {};

template <typename ColT, typename IndexT>
struct InsertMsg : ::vt::Message {
  InsertMsg() = default;

  InsertMsg(
    CollectionIndexProxy<ColT,IndexT> in_proxy,
    IndexT in_max, IndexT in_idx, NodeType in_construct_node,
    NodeType in_home_node, EpochType in_epoch, EpochType in_g_epoch
  ) : proxy_(in_proxy), max_(in_max), idx_(in_idx),
      construct_node_(in_construct_node), home_node_(in_home_node),
      epoch_(in_epoch), g_epoch_(in_g_epoch)
  { }

  CollectionIndexProxy<ColT,IndexT> proxy_ = {};
  IndexT max_ = {}, idx_ = {};
  NodeType construct_node_ = uninitialized_destination;
  NodeType home_node_ = uninitialized_destination;
  EpochType epoch_ = no_epoch;
  EpochType g_epoch_ = no_epoch;
};

template <typename ColT, typename IndexT>
struct DoneInsertMsg : ::vt::Message {
  DoneInsertMsg() = default;

  DoneInsertMsg(
    CollectionIndexProxy<ColT,IndexT> in_proxy,
    NodeType const& in_action_node = uninitialized_destination
  ) : action_node_(in_action_node), proxy_(in_proxy)
  { }

  NodeType action_node_ = uninitialized_destination;
  CollectionIndexProxy<ColT,IndexT> proxy_ = {};
};

template <typename ColT, typename IndexT>
struct ActInsertMsg : ::vt::Message {
  ActInsertMsg() = default;

  explicit ActInsertMsg(CollectionIndexProxy<ColT,IndexT> in_proxy)
    : proxy_(in_proxy)
  { }

  CollectionIndexProxy<ColT,IndexT> proxy_ = {};
};

template <typename ColT, typename IndexT>
struct UpdateInsertMsg : ::vt::Message {
  UpdateInsertMsg() = default;

  UpdateInsertMsg(
    CollectionIndexProxy<ColT,IndexT> in_proxy, EpochType const& in_epoch
  ) : proxy_(in_proxy), epoch_(in_epoch)
  { }

  CollectionIndexProxy<ColT,IndexT> proxy_ = {};
  EpochType epoch_ = no_epoch;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H*/
