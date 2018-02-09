
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H
#define INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H

#include "config.h"
#include "topos/location/location_headers.h"
#include "context/context.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/manager.h"
#include "vrt/collection/messages/system_create.h"
#include "vrt/collection/collection_info.h"
#include "vrt/collection/messages/user.h"
#include "vrt/collection/types/type_attorney.h"
#include "vrt/proxy/collection_wrapper.h"
#include "registry/auto_registry_map.h"
#include "registry/auto_registry_collection.h"
#include "topos/mapping/mapping_headers.h"

#include <tuple>
#include <utility>
#include <cassert>
#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename CollectionT, typename IndexT, typename Tuple, size_t... I>
/*static*/ typename CollectionManager::VirtualPtrType<IndexT>
CollectionManager::runConstructor(
  VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
  std::index_sequence<I...>
) {
  return std::make_unique<CollectionT>(
    /*elms, */idx,
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
  VirtualProxyType new_proxy = info.getProxy();

  theCollection()->insertCollectionInfo(new_proxy);

  if (info.immediate_) {
    auto const& map_han = msg->map;
    auto fn = auto_registry::getAutoHandlerMap(map_han);

    debug_print(
      vrt_coll, node,
      "running foreach: size=%llu, %d\n",
      info.range_.getSize(), info.range_.x()
    );

    auto const& user_index_range = info.getRange();

    user_index_range.foreach(user_index_range, [=](IndexT cur_idx) {
      auto mapped_node = fn(
        reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
        reinterpret_cast<vt::index::BaseIndex*>(&msg->info.range_),
        theContext()->getNumNodes()
      );

      debug_print(
        vrt_coll, node,
        "running foreach: node=%d, cur_idx.x()=%d\n",
        mapped_node, cur_idx.x()
      );

      if (node == mapped_node) {
        // need to construct elements here
        auto const& num_elms = info.range_.getSize();
        auto new_vc = CollectionManager::runConstructor<CollectionT, IndexT>(
          num_elms, cur_idx, &msg->tup, std::make_index_sequence<size>{}
        );
        CollectionTypeAttorney::setSize(new_vc, num_elms);
        theCollection()->insertCollectionElement<IndexT>(
          std::move(new_vc), cur_idx, msg->info.range_, map_han, new_proxy
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

  auto const& col = entity_proxy.getCollectionProxy();
  auto const& elm = entity_proxy.getElementProxy();
  auto const& idx = elm.getIndex();
  bool const& exists = Holder<IndexT>::exists(col, idx);
  assert(exists && "Proxy must exist");

  auto& inner_holder = Holder<IndexT>::lookup(col, idx);

  auto const sub_handler = col_msg->getVrtHandler();
  auto const collection_active_fn = auto_registry::getAutoHandlerCollection(
    sub_handler
  );
  auto const col_ptr = inner_holder.getCollection();

  debug_print(
    vrt_coll, node,
    "collectionMsgHandler: sub_handler=%d\n", sub_handler
  );

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
  VirtualElmProxyType<typename CollectionT::IndexType> const& toProxy,
  MessageT *const msg, ActionType act
) {
  using IndexT = typename CollectionT::IndexType;

  // @todo: implement the action `act' after the routing is finished

  auto const& col_proxy = toProxy.getCollectionProxy();
  auto const& elm_proxy = toProxy.getElementProxy();

  auto& holder_container = EntireHolder<IndexT>::proxy_container_;
  auto holder = holder_container.find(col_proxy);
  if (holder != holder_container.end()) {
    auto const map_han = holder->second.map_fn;
    auto max_idx = holder->second.max_idx;
    auto cur_idx = elm_proxy.getIndex();
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
      vrt_coll, node,
      "sending msg to collection: msg=%p, han=%d, home_node=%d\n",
      msg, han, home_node
    );

    // route the message to the destination using the location manager
    theLocMan()->getCollectionLM<IndexT>(col_proxy)->routeMsg(
      toProxy, home_node, msg, act
    );
  } else {
    auto iter = buffered_sends_.find(toProxy.getCollectionProxy());
    if (iter == buffered_sends_.end()) {
      buffered_sends_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(toProxy.getCollectionProxy()),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_sends_.find(toProxy.getCollectionProxy());
    }
    assert(iter != buffered_sends_.end() and "Must exist");

    debug_print(
      vrt_coll, node,
      "pushing into buffered sends: %lld\n", toProxy.getCollectionProxy()
    );

    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      theCollection()->sendMsg<CollectionT, MessageT, f>(toProxy, msg, act);
    });
  }
}

template <typename IndexT>
void CollectionManager::insertCollectionElement(
  VirtualPtrType<IndexT> vc, IndexT const& idx, IndexT const& max_idx,
  HandlerType const& map_han, VirtualProxyType const &proxy
) {
  auto& holder_container = EntireHolder<IndexT>::proxy_container_;
  auto holder_iter = holder_container.find(proxy);
  if (holder_iter == holder_container.end()) {
    holder_container.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(proxy),
      std::forward_as_tuple(
        typename EntireHolder<IndexT>::InnerHolder{map_han, max_idx}
      )
    );

    debug_print(
      vrt_coll, node,
      "looking for buffered sends: proxy=%lld, size=%ld\n",
      proxy, buffered_sends_.size()
    );

    auto iter = buffered_sends_.find(proxy);
    if (iter != buffered_sends_.end()) {
      debug_print(
        vrt_coll, node,
        "looking for buffered sends: FOUND\n"
      );

      for (auto&& elm : iter->second) {
        debug_print(
          vrt_coll, node,
          "looking for buffered sends: running elm\n"
        );

        elm(proxy);
      }
      iter->second.clear();
      buffered_sends_.erase(iter);
    }
  }

  auto const& elm_exists = Holder<IndexT>::exists(proxy, idx);
  assert(!elm_exists && "Must not exist at this point");

  Holder<IndexT>::insert(proxy, idx, typename Holder<IndexT>::InnerHolder{
    std::move(vc), map_han, max_idx
  });

  theLocMan()->getCollectionLM<IndexT>(proxy)->registerEntity(
    VrtElmProxy<IndexT>{proxy,idx},
    CollectionManager::collectionMsgHandler<IndexT>
  );
}

template <
  typename CollectionT,
  mapping::ActiveMapTypedFnType<typename CollectionT::IndexType> fn,
  typename... Args
>
CollectionManager::CollectionProxyWrapType<typename CollectionT::IndexType>
CollectionManager::makeCollection(
  typename CollectionT::IndexType const& range, Args&& ... args
) {
  using IndexT = typename CollectionT::IndexType;
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

  SerializedMessenger::broadcastSerialMsg<
    MsgType, createCollectionHan<MsgType>
  >(create_msg);

  CollectionManager::createCollectionHan<MsgType>(create_msg);

  messageDeref(create_msg);

  return CollectionProxyWrapType<typename CollectionT::IndexType>{new_proxy};
}

inline void CollectionManager::insertCollectionInfo(VirtualProxyType const& p) {
  // do nothing right now
}

inline VirtualProxyType CollectionManager::makeNewCollectionProxy() {
  auto const& node = theContext()->getNode();
  return VirtualProxyBuilder::createProxy(curIdent_++, node, true, true);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H*/
