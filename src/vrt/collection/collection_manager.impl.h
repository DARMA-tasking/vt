
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_IMPL_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_IMPL_H

#include "config.h"
#include "context/context.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/collection_manager.h"
#include "vrt/collection/collection_create_msg.h"
#include "vrt/collection/collection_info.h"
#include "vrt/collection/collection_msg.h"
#include "registry/auto_registry_map.h"
#include "registry/auto_registry_collection.h"
#include "topos/mapping/mapping_headers.h"

#include <tuple>
#include <utility>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename CollectionT, typename IndexT, typename Tuple, size_t... I>
/*static*/ typename CollectionManager::VirtualPtrType<IndexT>
CollectionManager::runConstructor(
  VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
  std::index_sequence<I...>
) {
  return std::make_unique<CollectionT>(
    elms, idx,
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(*tup)
    )...
  );
}

template <typename SysMsgT>
/*static*/ void CollectionManager::createCollectionHan(SysMsgT* msg) {
  using CollectionT = typename SysMsgT::CollectionType;
  using IndexT = typename SysMsgT::IndexType;
  using Args = typename SysMsgT::ArgsTupleType;

  static constexpr auto size = std::tuple_size<Args>::value;

  auto const& node = theContext()->getNode();
  auto& info = msg->info;
  VirtualProxyType new_proxy = info.proxy;

  theCollection()->insertCollectionInfo(new_proxy);

  if (info.isImmediate) {
    auto const& map_han = msg->map;
    auto fn = auto_registry::getAutoHandlerMap(map_han);

    printf("running foreach: size=%llu, %d\n", info.range.getSize(), info.range.x());

    info.range.foreach(info.range, [=](IndexT cur_idx) {
      auto mapped_node = fn(
        reinterpret_cast<vt::index::BaseIndex*>(&msg->info.range),
        reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
        theContext()->getNumNodes()
      );

      //printf("running foreach: node=%d, cur_idx.x()=%d\n", mapped_node, cur_idx.x());

      if (node == mapped_node) {
        // need to construct elements here
        auto new_vc = CollectionManager::runConstructor<CollectionT, IndexT>(
          info.range.getSize(), cur_idx, &msg->tup,
          std::make_index_sequence<size>{}
        );
        theCollection()->insertCollectionElement<IndexT>(
          std::move(new_vc), cur_idx, msg->info.range, map_han, new_proxy
        );
      }
    });
  } else {
    // just wait and register the proxy
  }
}

template <typename IndexT>
/*static*/ void CollectionManager::collectionMsgHandler(BaseMessage* msg) {
  auto const col_msg = static_cast<CollectionMessage<IndexT>*>(msg);
  auto const entity_proxy = col_msg->getProxy();

  auto& holder = CollectionHolder<IndexT>::vc_container_;
  auto proxy_holder = holder.find(entity_proxy.colProxy);
  assert(proxy_holder != holder.end() and "Proxy must exist");

  auto idx_holder = proxy_holder->second.find(entity_proxy.elmProxy);
  assert(idx_holder != proxy_holder->second.end() and "Idx proxy must exist");

  auto& inner_holder = idx_holder->second;

  auto const sub_handler = col_msg->getVrtHandler();
  auto const collection_active_fn = auto_registry::getAutoHandlerCollection(sub_handler);
  auto const col_ptr = inner_holder.getCollection();

  printf("collectionMsgHandler: sub_handler=%d\n", sub_handler);

  assert(col_ptr != nullptr && "Must be valid pointer");

  // for now, execute directly on comm thread
  collection_active_fn(msg, col_ptr);
}

template <
  typename CollectionT,
  typename MessageT,
  ActiveCollectionTypedFnType<MessageT, CollectionT> *f
>
void CollectionManager::sendMsg(
  VirtualElmProxyType const& toProxy, MessageT *const msg, ActionType act
) {
  using IndexT = typename CollectionT::IndexType;

  // @todo: implement the action `act' after the routing is finished

  auto& holder_container = CollectionEntireHolder<IndexT>::proxy_container_;
  auto holder = holder_container.find(toProxy.colProxy);
  if (holder == holder_container.end()) {
    auto const map_han = holder->second.map_fn;
    auto max_idx = holder->second.max_idx;
    auto cur_idx = IndexT::uniqueBitsToIndex(
      VirtualElemProxyBuilder::elmProxyGetIndex(toProxy.elmProxy)
    );
    auto fn = auto_registry::getAutoHandlerMap(map_han);

    auto const& num_nodes = theContext()->getNumNodes();

    auto home_node = fn(
      reinterpret_cast<vt::index::BaseIndex*>(&max_idx),
      reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
      num_nodes
    );

    // register the user's handler
    HandlerType const& han = auto_registry::makeAutoHandlerCollection<
      CollectionT, MessageT, f
    >(msg);

    // save the user's handler in the message
    msg->setVrtHandler(han);
    msg->setProxy(toProxy);

    debug_print(
      vrt, node,
      "sending msg to collection: msg=%p, han=%d, home_node=%d\n",
      msg, han, home_node
    );

    // route the message to the destination using the location manager
    theLocMan()->collectionLoc->routeMsg(toProxy, home_node, msg, act);
  } else {
    // @todo: buffer the msg
    assert(0);
  }
}

template <typename IndexT>
void CollectionManager::insertCollectionElement(
  VirtualPtrType<IndexT> vc, IndexT const& idx, IndexT const& max_idx,
  HandlerType const& map_han, VirtualProxyType const &proxy
) {
  auto& holder_container = CollectionEntireHolder<IndexT>::proxy_container_;
  auto holder_iter = holder_container.find(proxy);
  if (holder_iter == holder_container.end()) {
    holder_container.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(proxy),
      std::forward_as_tuple(
        typename CollectionEntireHolder<IndexT>::InnerHolder{map_han, max_idx}
      )
    );
  }

  auto& holder = CollectionHolder<IndexT>::vc_container_;
  auto proxy_holder = holder.find(proxy);
  if (proxy_holder == holder.end()) {
    holder.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(proxy),
      std::forward_as_tuple(
        typename CollectionHolder<IndexT>::UntypedIndexContainer{}
      )
    );
    proxy_holder = holder.find(proxy);
  }

  assert(proxy_holder != holder.end() and "Must exist at this point");

  auto const& bits = idx.uniqueBits();
  auto idx_iter = proxy_holder->second.find(bits);

  bool const& idx_exists = idx_iter != proxy_holder->second.end();

  assert(not idx_exists and "Index must not exist at this point");

  if (!idx_exists) {
    proxy_holder->second.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(bits),
      std::forward_as_tuple(
        typename CollectionHolder<IndexT>::InnerHolder{
          std::move(vc), map_han, max_idx
        }
      )
    );

    theLocMan()->collectionLoc->registerEntity(
      VrtElmProxy{proxy,bits}, CollectionManager::collectionMsgHandler<IndexT>
    );
  } else {
    assert(0);
  }
}

template <
  typename CollectionT,
  typename IndexT,
  mapping::ActiveMapTypedFnType<IndexT> fn,
  typename... Args
>
VirtualProxyType CollectionManager::makeCollection(
  IndexT const& range, Args&& ... args
) {
  using ArgsTupleType = std::tuple<typename std::decay<Args>::type...>;
  using MsgType = CollectionCreateMsg<
    CollectionInfo<IndexT>, ArgsTupleType, CollectionT, IndexT
  >;

  auto const& node = theContext()->getNode();
  auto const& map_handler = auto_registry::makeAutoHandlerMap<IndexT, fn>();

  auto create_msg = makeSharedMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );

  auto const& new_proxy = makeNewCollectionProxy();
  auto const& is_static = CollectionT::isStaticSized();

  CollectionInfo<IndexT> info(range, is_static, node, new_proxy);
  create_msg->info = info;

  messageRef(create_msg);

  SerializedMessenger::broadcastSerialMsg<MsgType, createCollectionHan<MsgType>>(
    create_msg
  );

  CollectionManager::createCollectionHan<MsgType>(create_msg);

  messageDeref(create_msg);

  return new_proxy;
}

inline void CollectionManager::insertCollectionInfo(VirtualProxyType const& p) {
  // do nothing right now
}

inline VirtualProxyType CollectionManager::makeNewCollectionProxy() {
  auto const& node = theContext()->getNode();
  return VirtualProxyBuilder::createProxy(curIdent_++, node, true, true);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_IMPL_H*/
