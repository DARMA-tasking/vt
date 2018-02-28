
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
#include "vrt/collection/defaults/default_map.h"
#include "vrt/collection/constructor/coll_constructors_deref.h"
#include "vrt/proxy/collection_wrapper.h"
#include "registry/auto/auto_registry_map.h"
#include "registry/auto/auto_registry_collection.h"
#include "registry/auto/auto_registry_common.h"
#include "topos/mapping/mapping_headers.h"
#include "termination/term_headers.h"
#include "serialization/serialization.h"

#include <tuple>
#include <utility>
#include <cassert>
#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename Tuple, size_t... I>
/*static*/ typename CollectionManager::VirtualPtrType<IndexT>
CollectionManager::runConstructor(
  VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
  std::index_sequence<I...>
) {
  return std::make_unique<ColT>(
    idx,
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(*tup)
    )...
  );
}

template <typename SysMsgT>
/*static*/ void CollectionManager::distConstruct(SysMsgT* msg) {
  using ColT = typename SysMsgT::CollectionType;
  using IndexT = typename SysMsgT::IndexType;
  using Args = typename SysMsgT::ArgsTupleType;

  static constexpr auto size = std::tuple_size<Args>::value;

  auto const& node = theContext()->getNode();
  auto& info = msg->info;
  VirtualProxyType new_proxy = info.getProxy();

  theCollection()->insertCollectionInfo(new_proxy);

  if (info.immediate_) {
    auto const& map_han = msg->map;
    bool const& is_functor =
      auto_registry::HandlerManagerType::isHandlerFunctor(map_han);

    auto_registry::AutoActiveMapType fn = nullptr;

    if (is_functor) {
      fn = auto_registry::getAutoHandlerFunctorMap(map_han);
    } else {
      fn = auto_registry::getAutoHandlerMap(map_han);
    }

    debug_print(
      vrt_coll, node,
      "running foreach: size=%llu, %d, is_functor=%s\n",
      info.range_.getSize(), info.range_.x(), print_bool(is_functor)
    );

    auto user_index_range = info.getRange();
    auto max_range = msg->info.range_;

    user_index_range.foreach(user_index_range, [=](IndexT cur_idx) mutable {
      debug_print(
        verbose, vrt_coll, node,
        "running foreach: before map: cur_idx=%s, max_range=%s\n",
        cur_idx.toString().c_str(), max_range.toString().c_str()
      );

      auto mapped_node = fn(
        reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
        reinterpret_cast<vt::index::BaseIndex*>(&max_range),
        theContext()->getNumNodes()
      );

      debug_print(
        vrt_coll, node,
        "running foreach: node=%d, cur_idx=%s, max_range=%s\n",
        mapped_node, cur_idx.toString().c_str(), max_range.toString().c_str()
      );

      if (node == mapped_node) {
        // need to construct elements here
        auto const& num_elms = info.range_.getSize();

        #if backend_check_enabled(detector)
          auto new_vc = DerefCons::derefTuple<ColT, IndexT, decltype(msg->tup)>(
            num_elms, cur_idx, &msg->tup
          );
        #else
          auto new_vc = CollectionManager::runConstructor<ColT, IndexT>(
            num_elms, cur_idx, &msg->tup, std::make_index_sequence<size>{}
          );
        #endif

        /*
         * Set direct attributes of the newly constructed element directly on
         * the user's class
         */
        CollectionTypeAttorney::setSize(new_vc, num_elms);
        CollectionTypeAttorney::setProxy(new_vc, new_proxy);
        CollectionTypeAttorney::setIndex<decltype(new_vc),IndexT>(
          new_vc, cur_idx
        );

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

  // Be careful with type casting here..convert to typeless before
  // reinterpreting the pointer so the compiler does not produce the wrong
  // offset
  auto const col_ptr = inner_holder.getCollection();
  void* typeless_collection = static_cast<void*>(col_ptr);

  debug_print(
    vrt_coll, node,
    "collectionMsgHandler: sub_handler=%d\n", sub_handler
  );

  assert(col_ptr != nullptr && "Must be valid pointer");

  // for now, execute directly on comm thread
  collection_active_fn(
    msg, reinterpret_cast<UntypedCollection*>(typeless_collection)
  );

  theTerm()->consume(term::any_epoch_sentinel);
}

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
void CollectionManager::sendMsg(
  VirtualElmProxyType<typename ColT::IndexType> const& toProxy,
  MsgT *const msg, ActionType act
) {
  using IndexT = typename ColT::IndexType;

  // @todo: implement the action `act' after the routing is finished

  auto const& col_proxy = toProxy.getCollectionProxy();
  auto const& elm_proxy = toProxy.getElementProxy();

  theTerm()->produce(term::any_epoch_sentinel);

  auto& holder_container = EntireHolder<IndexT>::proxy_container_;
  auto holder = holder_container.find(col_proxy);
  if (holder != holder_container.end()) {
    auto const map_han = holder->second.map_fn;
    auto max_idx = holder->second.max_idx;
    auto cur_idx = elm_proxy.getIndex();
    auto fn = auto_registry::getAutoHandlerMap(map_han);

    auto const& num_nodes = theContext()->getNumNodes();

    auto home_node = fn(
      reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
      reinterpret_cast<vt::index::BaseIndex*>(&max_idx),
      num_nodes
    );

    // register the user's handler
    HandlerType const& han = auto_registry::makeAutoHandlerCollection<
      ColT, MsgT, f
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
    auto lm = theLocMan()->getCollectionLM<IndexT>(col_proxy);
    assert(lm != nullptr && "LM must exist");
    lm->routeMsg(toProxy, home_node, msg, act);
    // TODO: race when proxy gets transferred off node before the LM is created
    // LocationManager::applyInstance<LocationManager::VrtColl<IndexT>>
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

    theTerm()->produce(term::any_epoch_sentinel);

    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      theCollection()->sendMsg<ColT, MsgT, f>(toProxy, msg, act);
    });
  }
}

template <typename IndexT>
void CollectionManager::insertCollectionElement(
  VirtualPtrType<IndexT> vc, IndexT const& idx, IndexT const& max_idx,
  HandlerType const& map_han, VirtualProxyType const& proxy
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

        theTerm()->consume(term::any_epoch_sentinel);

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

template <typename ColT, typename... Args>
CollectionManager::CollectionProxyWrapType<typename ColT::IndexType>
CollectionManager::construct(
  typename ColT::IndexType range, Args&&... args
) {
  using ParamT = typename DefaultMap<ColT>::MapParamPackType;
  auto const& map_han = auto_registry::makeAutoHandlerFunctorMap<
    typename DefaultMap<ColT>::MapType,
    typename std::tuple_element<0,ParamT>::type,
    typename std::tuple_element<1,ParamT>::type,
    typename std::tuple_element<2,ParamT>::type
  >();
  return constructMap<ColT,Args...>(range,map_han,args...);
}

template <
  typename ColT, mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn,
  typename... Args
>
CollectionManager::CollectionProxyWrapType<typename ColT::IndexType>
CollectionManager::construct(
  typename ColT::IndexType range, Args&&... args
) {
  using IndexT = typename ColT::IndexType;
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructMap<ColT,Args...>(range, map_han, args...);
}

template <typename ColT, typename... Args>
CollectionManager::CollectionProxyWrapType<typename ColT::IndexType>
CollectionManager::constructMap(
  typename ColT::IndexType range, HandlerType const& map_handler,
  Args&&... args
) {
  using SerdesMsg = SerializedMessenger;
  using IndexT = typename ColT::IndexType;
  using ArgsTupleType = std::tuple<typename std::decay<Args>::type...>;
  using MsgType = CollectionCreateMsg<
    CollectionInfo<IndexT>, ArgsTupleType, ColT, IndexT
  >;

  auto const& new_proxy = makeNewCollectionProxy();
  auto const& is_static = ColT::isStaticSized();
  auto lm = theLocMan()->getCollectionLM<IndexT>(new_proxy);
  auto const& node = theContext()->getNode();
  auto create_msg = makeSharedMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );

  CollectionInfo<IndexT> info(range, is_static, node, new_proxy);
  create_msg->info = info;

  debug_print(
    vrt_coll, node,
    "construct_map: range=%s\n", range.toString().c_str()
  );

  SerdesMsg::broadcastSerialMsg<MsgType, distConstruct<MsgType>>(create_msg);

  auto create_msg_local = makeSharedMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );
  create_msg_local->info = info;
  CollectionManager::distConstruct<MsgType>(create_msg_local);
  messageDeref(create_msg_local);

  return CollectionProxyWrapType<typename ColT::IndexType>{new_proxy};
}

inline void CollectionManager::insertCollectionInfo(VirtualProxyType const& p) {
  // do nothing right now
}

inline VirtualProxyType CollectionManager::makeNewCollectionProxy() {
  auto const& node = theContext()->getNode();
  return VirtualProxyBuilder::createProxy(curIdent_++, node, true, true);
}

/*
 * Support of virtual context collection element migration
 */

template <typename IndexT>
MigrateStatus CollectionManager::migrate(
  VirtualProxyType const& col_proxy, IndexT const& idx, NodeType const& node
) {
 debug_print(
   vrt_coll, node,
   "migrate: col_proxy=%llu, node=%d\n", col_proxy, node
 );
 printf("migrate: col_proxy=%llu, node=%d\n", col_proxy, node);

 auto const& this_node = theContext()->getNode();
 if (this_node != node) {
   auto const& proxy = CollectionIndexProxy<IndexT>(col_proxy).operator()(idx);

   auto& proxy_cont_iter = EntireHolder<IndexT>::proxy_container_;
   auto holder_iter = proxy_cont_iter.find(col_proxy);
   assert(
     holder_iter != proxy_cont_iter.end() && "Element must be registered here"
   );

   #if backend_check_enabled(runtime_checks)
   {
     bool const& exists = Holder<IndexT>::exists(col_proxy, idx);
     assert(
       exists && "Local element must exist here for migration to occur"
     );
   }
   #endif

   auto& inner_holder = Holder<IndexT>::lookup(col_proxy, idx);
   auto const col_ptr = inner_holder.getCollection();

   // ::serialization::interface::serialize(
   //   col_ptr, [](SizeType size) -> SerialByteType* {
   //   }
   // );

   theLocMan()->getCollectionLM<IndexT>(col_proxy)->entityMigrated(proxy, node);

   return MigrateStatus::MigratedToRemote;
 } else {
   #if backend_check_enabled(runtime_checks)
     assert(
       false && "Migration should only be called when node is != this_node"
     );
   #else
     // Do nothing
   #endif
   return MigrateStatus::NoMigrationNecessary;
 }
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H*/
