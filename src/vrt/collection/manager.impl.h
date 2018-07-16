
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
#include "vrt/collection/migrate/migrate_msg.h"
#include "vrt/collection/migrate/migrate_handlers.h"
#include "vrt/collection/active/active_funcs.h"
#include "vrt/collection/destroy/destroy_msg.h"
#include "vrt/collection/destroy/destroy_handlers.h"
#include "vrt/collection/balance/phase_msg.h"
#include "vrt/proxy/collection_wrapper.h"
#include "registry/auto/map/auto_registry_map.h"
#include "registry/auto/collection/auto_registry_collection.h"
#include "registry/auto/auto_registry_common.h"
#include "topos/mapping/mapping_headers.h"
#include "termination/term_headers.h"
#include "serialization/serialization.h"
#include "serialization/auto_dispatch/dispatch.h"
#include "collective/reduce/reduce_hash.h"
#include "runnable/collection.h"

#include <tuple>
#include <utility>
#include <functional>
#include <cassert>
#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename>
/*static*/ VirtualIDType CollectionManager::curIdent_ = 0;

template <typename>
void CollectionManager::destroyCollections() {
  UniversalIndexHolder<>::destroyAllLive();
}

template <typename ColT, typename IndexT, typename Tuple, size_t... I>
/*static*/ typename CollectionManager::VirtualPtrType<ColT, IndexT>
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
  auto const& insert_epoch = info.getInsertEpoch();
  bool element_created_here = false;

  theCollection()->insertCollectionInfo(new_proxy,msg->map,insert_epoch);

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
      "running foreach: size={}, {}, is_functor={}\n",
      info.range_.getSize(), info.range_.x(), print_bool(is_functor)
    );

    auto user_index_range = info.getRange();
    auto max_range = msg->info.range_;

    user_index_range.foreach(user_index_range, [&](IndexT cur_idx) mutable {
      debug_print(
        verbose, vrt_coll, node,
        "running foreach: before map: cur_idx={}, max_range={}\n",
        cur_idx.toString().c_str(), max_range.toString().c_str()
      );

      auto mapped_node = fn(
        reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
        reinterpret_cast<vt::index::BaseIndex*>(&max_range),
        theContext()->getNumNodes()
      );

      debug_print(
        vrt_coll, node,
        "running foreach: node={}, cur_idx={}, max_range={}\n",
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

        theCollection()->insertCollectionElement<ColT, IndexT>(
          std::move(new_vc), cur_idx, msg->info.range_, map_han, new_proxy,
          mapped_node
        );
        element_created_here = true;
      }
    });
  } else {
    assert(element_created_here == false && "Element should not be created");
  }

  if (!element_created_here) {
    auto const& map_han = msg->map;
    auto const& max_idx = msg->info.range_;
    using HolderType = typename EntireHolder<ColT, IndexT>::InnerHolder;
    EntireHolder<ColT, IndexT>::insert(
      new_proxy, std::make_shared<HolderType>(map_han,max_idx)
    );
    /*
     *  This is to ensure that the collection LM instance gets created so that
     *  messages can be forwarded properly
     */
    theLocMan()->getCollectionLM<ColT, IndexT>(new_proxy);
  }

  uint64_t const tag_mask_ = 0x0ff00000;
  auto const& vid = VirtualProxyBuilder::getVirtualID(new_proxy);
  auto construct_msg = makeSharedMessage<CollectionConsMsg>(new_proxy);
  auto const& tag_id = vid | tag_mask_;
  auto const& root = 0;
  debug_print(
    vrt_coll, node,
    "calling reduce: new_proxy={}\n", new_proxy
  );
  theCollective()->reduce<CollectionConsMsg,collectionConstructHan>(
    root, construct_msg, tag_id
  );

  /*
   *  Create a new group for the collection that only contains the nodes for
   *  which elements exist. If the collection is static, this group will never
   *  change
   */

  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(new_proxy);
  std::size_t const num_elms = elm_holder->numElements();
  bool const in_group = num_elms > 0;

  debug_print(
    vrt_coll, node,
    "constructing group: num_elms={}, in_group={}\n",
    num_elms, in_group
  );

  auto const group_id = theGroup()->newGroupCollective(
    in_group, [new_proxy,vid](GroupType new_group){
      auto const& group_root = theGroup()->groupRoot(new_group);
      auto const& is_group_default = theGroup()->groupDefault(new_group);
      auto const& in_group = theGroup()->inGroup(new_group);
      auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(new_proxy);
      elm_holder->setGroup(new_group);
      elm_holder->setUseGroup(!is_group_default);
      elm_holder->setGroupReady(true);
      if (!is_group_default) {
        elm_holder->setGroupRoot(group_root);
      }

      debug_print(
        vrt_coll, node,
        "group finished construction: new_group={:x}, use_group={}, ready={}, "
        "root={}, is_group_default={}\n",
        new_group, elm_holder->useGroup(), elm_holder->groupReady(),
        group_root, is_group_default
      );

      if (!is_group_default && in_group) {
        uint64_t const group_tag_mask = 0x0fff0000;
        auto group_msg = makeSharedMessage<CollectionConsMsg>(new_proxy);
        auto const& group_tag_id = vid | group_tag_mask;
        debug_print(
          vrt_coll, node,
          "calling group (construct) reduce: new_proxy={}\n", new_proxy
        );
        theGroup()->groupReduce(new_group)->reduce<
          CollectionConsMsg,collectionGroupConstructHan
        >(group_root, group_msg, group_tag_id);
      }
    }
  );

  debug_print(
    vrt_coll, node,
    "called newGroupCollective: num_elms={}, in_group={}, group_id={:x}\n",
    num_elms, in_group, group_id
  );
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::collectionBcastHandler(MsgT* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const col = col_msg->getBcastProxy();
  debug_print(
    vrt_coll, node,
    "collectionBcastHandler: proxy={}, han={}, epoch={}\n",
    col, col_msg->getVrtHandler(), col_msg->getBcastEpoch()
  );
  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(col);
  if (elm_holder) {
    auto const handler = col_msg->getVrtHandler();
    auto const member = col_msg->getMember();
    debug_print(
      vrt_coll, node,
      "broadcast apply: size={}\n", elm_holder->numElements()
    );
    elm_holder->foreach([col_msg,msg,handler,member](
      IndexT const& idx, CollectionBase<ColT,IndexT>* base
    ) {
      debug_print(
        vrt_coll, node,
        "broadcast: apply to element: epoch={}, bcast_epoch={}\n",
        msg->bcast_epoch_, base->cur_bcast_epoch_
      );
      if (base->cur_bcast_epoch_ == msg->bcast_epoch_ - 1) {
        void* typeless_collection = static_cast<void*>(base);
        assert(base != nullptr && "Must be valid pointer");
        // be very careful here, do not touch `base' after running the active
        // message because it might have migrated out and be invalid
        base->cur_bcast_epoch_++;

        backend_enable_if(
          lblite, {
            debug_print(
              vrt_coll, node,
              "broadcast: apply to element: instrument={}\n",
              msg->lbLiteInstrument()
            );

            if (msg->lbLiteInstrument()) {
              auto& stats = base->getStats();
              stats.startTime();
            }
          }
        );

        auto const from_node = col_msg->getFromNode();
        auto untyped = reinterpret_cast<UntypedCollection*>(typeless_collection);
        runnable::RunnableCollection<MsgT,UntypedCollection>::run(
          handler, msg, untyped, from_node, member,
          *reinterpret_cast<uint64_t const*>(base->getIndex().raw())
        );

        backend_enable_if(
          lblite, {
            if (msg->lbLiteInstrument()) {
              auto& stats = base->getStats();
              stats.stopTime();
            }
          }
        );
      }
    });
  }
  theTerm()->consume(term::any_epoch_sentinel);
}

template <typename>
/*static*/ void CollectionManager::collectionGroupFinishedHan(
  CollectionConsMsg* msg
) {

}

template <typename>
/*static*/ void CollectionManager::collectionFinishedHan(
  CollectionConsMsg* msg
) {
  auto const& proxy = msg->proxy;
  theCollection()->constructed_.insert(proxy);
  auto& buffered = theCollection()->buffered_bcasts_;
  auto iter = buffered.find(proxy);
  debug_print(
    vrt_coll, node,
    "collectionFinishedHan: proxy={}, buffered size={}\n",
    proxy, (iter != buffered.end() ? iter->second.size() : -1)
  );
  if (iter != buffered.end()) {
    for (auto&& elm : iter->second) {
      elm(proxy);
    }
    iter->second.clear();
    buffered.erase(iter);
  }
}

template <typename>
/*static*/ void CollectionManager::collectionConstructHan(
  CollectionConsMsg* msg
) {
  debug_print(
    vrt_coll, node,
    "collectionConstructHan: proxy={}\n", msg->proxy
  );
  if (msg->isRoot()) {
    auto new_msg = makeSharedMessage<CollectionConsMsg>(*msg);
    messageRef(new_msg);
    theMsg()->broadcastMsg<CollectionConsMsg,collectionFinishedHan>(new_msg);
    collectionFinishedHan(new_msg);
    messageDeref(new_msg);
  } else {
    // do nothing
  }
}

template <typename>
/*static*/ void CollectionManager::collectionGroupConstructHan(
  CollectionConsMsg* msg
) {
  debug_print(
    vrt_coll, node,
    "collectionGroupConstructHan: proxy={}, root={}\n",
    msg->proxy, msg->isRoot()
  );
  if (msg->isRoot()) {
    auto new_msg = makeSharedMessage<CollectionConsMsg>(*msg);
    //theMsg()->broadcastMsg<CollectionConsMsg,collectionGroupFinishedHan>(new_msg);
  } else {
    // do nothing
  }
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::collectionMsgHandler(BaseMessage* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const entity_proxy = col_msg->getProxy();

  auto const& col = entity_proxy.getCollectionProxy();
  auto const& elm = entity_proxy.getElementProxy();
  auto const& idx = elm.getIndex();
  auto elm_holder = theCollection()->findElmHolder<ColT, IndexT>(col);

  bool const& exists = elm_holder->exists(idx);

  debug_print(
    vrt_coll, node,
    "collectionMsgHandler: exists={}, idx={}\n",
    exists, idx
  );
  assert(exists && "Proxy must exist");

  auto& inner_holder = elm_holder->lookup(idx);

  auto const sub_handler = col_msg->getVrtHandler();
  auto const member = col_msg->getMember();

  // Be careful with type casting here..convert to typeless before
  // reinterpreting the pointer so the compiler does not produce the wrong
  // offset
  auto const col_ptr = inner_holder.getCollection();
  void* typeless_collection = static_cast<void*>(col_ptr);

  debug_print(
    vrt_coll, node,
    "collectionMsgHandler: sub_handler={}\n", sub_handler
  );

  assert(col_ptr != nullptr && "Must be valid pointer");

  backend_enable_if(
    lblite, {
      debug_print(
        vrt_coll, node,
        "collectionMsgHandler: receive msg: instrument={}\n",
        col_msg->lbLiteInstrument()
      );
      if (col_msg->lbLiteInstrument()) {
        auto& stats = col_ptr->getStats();
        stats.startTime();
      }
    }
  );

  auto const from_node = col_msg->getFromNode();
  auto untyped = reinterpret_cast<UntypedCollection*>(typeless_collection);
  runnable::RunnableCollection<CollectionMessage<ColT>,UntypedCollection>::run(
    sub_handler, col_msg, untyped, from_node, member,
    *reinterpret_cast<uint64_t const*>(col_ptr->getIndex().raw())
  );

  backend_enable_if(
    lblite, {
      if (col_msg->lbLiteInstrument()) {
        auto& stats = col_ptr->getStats();
        stats.stopTime();
      }
    }
  );

  theTerm()->consume(term::any_epoch_sentinel);
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::broadcastRootHandler(MsgT* msg) {
  theCollection()->broadcastFromRoot<ColT,IndexT,MsgT>(msg);
}

template <typename ColT, typename IndexT, typename MsgT>
void CollectionManager::broadcastFromRoot(MsgT* msg) {
  // broadcast to all nodes
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& proxy = msg->getBcastProxy();
  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(proxy);
  auto const bcast_node = VirtualProxyBuilder::getVirtualNode(proxy);

  assert(elm_holder != nullptr && "Must have elm holder");
  assert(this_node == bcast_node && "Must be the bcast node");

  auto const epoch = elm_holder->cur_bcast_epoch_++;

  msg->setBcastEpoch(epoch);

  debug_print(
    vrt_coll, node,
    "broadcastFromRoot: proxy={}, epoch={}, han={}\n",
    proxy, msg->getBcastEpoch(), msg->getVrtHandler()
  );

  theTerm()->produce(term::any_epoch_sentinel, num_nodes);

  using Serial = ::vt::serialization::auto_dispatch::RequiredSerialization<
    MsgT, collectionBcastHandler<ColT,IndexT,MsgT>
  >;

  //auto new_msg = makeSharedMessage<MsgT>(*msg);
  messageRef(msg);
  Serial::broadcastMsg(msg);
  //theMsg()->broadcastMsg<MsgT,collectionBcastHandler<ColT,IndexT,MsgT>>(msg);
  collectionBcastHandler<ColT,IndexT,MsgT>(msg);
  messageDeref(msg);
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
>
void CollectionManager::broadcastMsg(
  CollectionProxyWrapType<
    typename MsgT::CollectionType, typename MsgT::CollectionType::IndexType
  > const& toProxy,
  MsgT *const msg, ActionType act, bool inst
) {
  // register the user's handler
  HandlerType const& han = auto_registry::makeAutoHandlerCollectionMem<
    typename MsgT::CollectionType, MsgT, f
  >(msg);
  return broadcastMsgUntypedHandler<MsgT>(toProxy,msg,han,true,act,inst);
}

template <
  typename MsgT,
  ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
>
void CollectionManager::broadcastMsg(
  CollectionProxyWrapType<
    typename MsgT::CollectionType, typename MsgT::CollectionType::IndexType
  > const& toProxy,
  MsgT *const msg, ActionType act, bool inst
) {
  // register the user's handler
  HandlerType const& han = auto_registry::makeAutoHandlerCollection<
    typename MsgT::CollectionType, MsgT, f
  >(msg);
  return broadcastMsgUntypedHandler<MsgT>(toProxy,msg,han,false,act,inst);
}

template <typename MsgT, typename ColT, typename IdxT>
void CollectionManager::broadcastMsgUntypedHandler(
  CollectionProxyWrapType<ColT, IdxT> const& toProxy, MsgT *msg,
  HandlerType const& handler, bool const member,
  ActionType act, bool instrument
) {
  debug_print(
    vrt_coll, node,
    "broadcastMsg: msg={}\n", print_ptr(msg)
  );

  auto const& this_node = theContext()->getNode();
  auto const& col_proxy = toProxy.getProxy();

  backend_enable_if(
    lblite,
    msg->setLBLiteInstrument(instrument);
  );

  // @todo: implement the action `act' after the routing is finished
  auto holder = findColHolder<ColT,IdxT>(col_proxy);
  auto found_constructed = constructed_.find(col_proxy) != constructed_.end();

  debug_print(
    vrt_coll, node,
    "broadcastMsg: col_proxy={}, found={}\n",
    col_proxy, found_constructed
  );

  if (holder != nullptr && found_constructed) {
    // save the user's handler in the message
    msg->setVrtHandler(handler);
    msg->setBcastProxy(col_proxy);
    msg->setFromNode(this_node);
    msg->setMember(member);

    auto const bcast_node = VirtualProxyBuilder::getVirtualNode(col_proxy);

    if (this_node != bcast_node) {
      debug_print(
        vrt_coll, node,
        "broadcastMsg: col_proxy={}, sending to root node={}, handler={}\n",
        col_proxy, bcast_node, handler
      );

      using Serial = ::vt::serialization::auto_dispatch::RequiredSerialization<
        MsgT, broadcastRootHandler<ColT,IdxT,MsgT>
      >;

      Serial::sendMsg(bcast_node, msg);
      // theMsg()->sendMsg<MsgT,broadcastRootHandler<ColT,IdxT,MsgT>>(
      //   bcast_node, msg
      // );
    } else {
      debug_print(
        vrt_coll, node,
        "broadcasting msg to collection: msg={}, handler={}\n",
        print_ptr(msg), handler
      );
      broadcastFromRoot<ColT,IdxT,MsgT>(msg);
    }
  } else {
    auto iter = buffered_bcasts_.find(col_proxy);
    if (iter == buffered_bcasts_.end()) {
      buffered_bcasts_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(col_proxy),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_bcasts_.find(col_proxy);
    }
    assert(iter != buffered_bcasts_.end() and "Must exist");

    debug_print(
      vrt_coll, node,
      "pushing into buffered sends: {}\n", col_proxy
    );

    theTerm()->produce(term::any_epoch_sentinel);

    debug_print(
      vrt_coll, node,
      "broadcastMsg: col_proxy={}, buffering\n", col_proxy
    );
    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      debug_print(
        vrt_coll, node,
        "broadcastMsg: col_proxy={}, running buffered\n", col_proxy
      );
      theTerm()->consume(term::any_epoch_sentinel);
      theCollection()->broadcastMsgUntypedHandler<MsgT,ColT,IdxT>(
        toProxy, msg, handler, member, act, instrument
      );
    });
  }
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType CollectionManager::reduceMsgExpr(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
  MsgT *const msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
  EpochType const& epoch, TagType const& tag
) {
  using IndexT = typename ColT::IndexType;

  debug_print(
    vrt_coll, node,
    "reduceMsg: msg={}\n", print_ptr(msg)
  );

  auto const& col_proxy = toProxy.getProxy();

  // @todo: implement the action `act' after the routing is finished
  auto found_constructed = constructed_.find(col_proxy) != constructed_.end();
  auto elm_holder = findElmHolder<ColT,IndexT>(col_proxy);

  debug_print(
    vrt_coll, node,
    "reduceMsg: col_proxy={}, found={}, group={:x}\n",
    col_proxy, found_constructed, elm_holder->group()
  );

  if (found_constructed && elm_holder) {
    std::size_t num_elms = 0;

    if (expr_fn == nullptr) {
      num_elms = elm_holder->numElements();
    } else {
      num_elms = elm_holder->numElementsExpr(expr_fn);
    }

    auto reduce_id = std::make_tuple(col_proxy,tag);
    auto epoch_iter = reduce_cur_epoch_.find(reduce_id);
    EpochType cur_epoch = epoch;
    if (epoch == no_epoch && epoch_iter != reduce_cur_epoch_.end()) {
      cur_epoch = epoch_iter->second;
    }
    auto ret_epoch = theCollective()->reduce<MsgT,f>(
      0,msg,tag,cur_epoch,num_elms,col_proxy
    );
    debug_print(
      vrt_coll, node,
      "reduceMsg: col_proxy={}, epoch={}, num_elms={}, tag={}\n",
      col_proxy, cur_epoch, num_elms, tag
    );
    if (epoch_iter == reduce_cur_epoch_.end()) {
      reduce_cur_epoch_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(reduce_id),
        std::forward_as_tuple(ret_epoch)
      );
    }

    return ret_epoch;
  } else {
    // @todo: implement this
    assert(0);
    return no_epoch;
  }
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType CollectionManager::reduceMsg(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
  MsgT *const msg, EpochType const& epoch, TagType const& tag
) {
  return reduceMsgExpr<ColT,MsgT,f>(toProxy,msg,nullptr,epoch,tag);
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
>
void CollectionManager::sendMsg(
  VirtualElmProxyType<
    typename MsgT::CollectionType, typename MsgT::CollectionType::IndexType
  > const& toProxy,
  MsgT *msg, ActionType action
) {
  // register the user's handler
  HandlerType const& handler = auto_registry::makeAutoHandlerCollectionMem<
    typename MsgT::CollectionType, MsgT, f
  >(msg);
  // save the user's handler in the message
  return sendMsgUntypedHandler<MsgT>(toProxy,msg,handler,true,action);
}

template <
  typename MsgT,
  ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
>
void CollectionManager::sendMsg(
  VirtualElmProxyType<
    typename MsgT::CollectionType, typename MsgT::CollectionType::IndexType
  > const& toProxy,
  MsgT *msg, ActionType action
) {
  // register the user's handler
  HandlerType const& handler = auto_registry::makeAutoHandlerCollection<
    typename MsgT::CollectionType, MsgT, f
  >(msg);
  // save the user's handler in the message
  return sendMsgUntypedHandler<MsgT>(toProxy,msg,handler,false,action);
}

template <typename MsgT, typename ColT, typename IdxT>
void CollectionManager::sendMsgUntypedHandler(
  VirtualElmProxyType<ColT, typename ColT::IndexType> const& toProxy,
  MsgT *msg, HandlerType const& handler, bool const member, ActionType action
) {
  // @todo: implement the action `action' after the routing is finished

  auto const& col_proxy = toProxy.getCollectionProxy();
  auto const& elm_proxy = toProxy.getElementProxy();

  backend_enable_if(
    lblite,
    msg->setLBLiteInstrument(true);
  );

  theTerm()->produce(term::any_epoch_sentinel);

  auto holder = findColHolder<ColT, IdxT>(col_proxy);
  if (holder != nullptr) {
    auto const map_han = holder->map_fn;
    auto max_idx = holder->max_idx;
    auto cur_idx = elm_proxy.getIndex();
    auto fn = auto_registry::getAutoHandlerMap(map_han);

    auto const& num_nodes = theContext()->getNumNodes();

    auto home_node = fn(
      reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
      reinterpret_cast<vt::index::BaseIndex*>(&max_idx),
      num_nodes
    );

    msg->setVrtHandler(handler);
    msg->setProxy(toProxy);
    msg->setMember(member);

    debug_print(
      vrt_coll, node,
      "sending msg to collection: msg={}, handler={}, home_node={}\n",
      print_ptr(msg), handler, home_node
    );

    // route the message to the destination using the location manager
    auto lm = theLocMan()->getCollectionLM<ColT, IdxT>(col_proxy);
    assert(lm != nullptr && "LM must exist");
    lm->routeMsgSerialize(toProxy, home_node, msg, action);
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
      "pushing into buffered sends: {}\n", toProxy.getCollectionProxy()
    );

    theTerm()->produce(term::any_epoch_sentinel);

    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      theCollection()->sendMsgUntypedHandler<MsgT,ColT,IdxT>(
        toProxy, msg, handler, member, action
      );
    });
  }
}

template <typename ColT, typename IndexT>
bool CollectionManager::insertCollectionElement(
  VirtualPtrType<ColT, IndexT> vc, IndexT const& idx, IndexT const& max_idx,
  HandlerType const& map_han, VirtualProxyType const& proxy,
  NodeType const& home_node, bool const& is_migrated_in,
  NodeType const& migrated_from
) {
  auto holder = findColHolder<ColT, IndexT>(proxy);

  debug_print(
    vrt_coll, node,
    "insertCollectionElement: proxy={}, map_han={}, idx={}, max_idx={}\n",
    proxy, map_han, print_index(idx), print_index(max_idx)
  );

  if (holder == nullptr) {
    using HolderType = typename EntireHolder<ColT, IndexT>::InnerHolder;

    EntireHolder<ColT, IndexT>::insert(
      proxy, std::make_shared<HolderType>(map_han,max_idx)
    );

    debug_print(
      vrt_coll, node,
      "looking for buffered sends: proxy={}, size={}\n",
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

  auto elm_holder = findElmHolder<ColT,IndexT>(proxy);
  auto const& elm_exists = elm_holder->exists(idx);
  assert(!elm_exists && "Must not exist at this point");

  auto const& destroyed = elm_holder->isDestroyed();

  if (!destroyed) {
    elm_holder->insert(idx, typename Holder<ColT, IndexT>::InnerHolder{
      std::move(vc), map_han, max_idx
    });

    if (is_migrated_in) {
      theLocMan()->getCollectionLM<ColT, IndexT>(proxy)->registerEntityMigrated(
        VrtElmProxy<ColT, IndexT>{proxy,idx}, migrated_from,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
      );
    } else {
      theLocMan()->getCollectionLM<ColT, IndexT>(proxy)->registerEntity(
        VrtElmProxy<ColT, IndexT>{proxy,idx}, home_node,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
      );
    }
    return true;
  } else {
    return false;
  }
}

template <typename ColT, typename... Args>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
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
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::construct(
  typename ColT::IndexType range, Args&&... args
) {
  using IndexT = typename ColT::IndexType;
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructMap<ColT,Args...>(range, map_han, args...);
}

template <typename ColT, typename... Args>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::constructMap(
  typename ColT::IndexType range, HandlerType const& map_handler,
  Args&&... args
) {
  using SerdesMsg = SerializedMessenger;
  using IndexT = typename ColT::IndexType;
  using ArgsTupleType = std::tuple<typename std::decay<Args>::type...>;
  using MsgType = CollectionCreateMsg<
    CollectionInfo<ColT, IndexT>, ArgsTupleType, ColT, IndexT
  >;

  auto const& new_proxy = makeNewCollectionProxy();
  auto const& is_static = ColT::isStaticSized();
  auto lm = theLocMan()->getCollectionLM<ColT, IndexT>(new_proxy);
  auto const& node = theContext()->getNode();
  auto create_msg = makeSharedMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );

  CollectionInfo<ColT, IndexT> info(range, is_static, node, new_proxy);

  if (!is_static) {
    auto const& insert_epoch = theTerm()->makeEpochRooted();
    info.setInsertEpoch(insert_epoch);
    setupNextInsertTrigger<ColT,IndexT>(new_proxy,insert_epoch);
  }

  create_msg->info = info;

  debug_print(
    vrt_coll, node,
    "construct_map: range={}\n", range.toString().c_str()
  );

  SerdesMsg::broadcastSerialMsg<MsgType, distConstruct<MsgType>>(create_msg);

  auto create_msg_local = makeSharedMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );
  create_msg_local->info = info;
  CollectionManager::distConstruct<MsgType>(create_msg_local);
  messageDeref(create_msg_local);

  return CollectionProxyWrapType<ColT, typename ColT::IndexType>{new_proxy};
}

inline void CollectionManager::insertCollectionInfo(
  VirtualProxyType const& proxy, HandlerType const& map_han,
  EpochType const& insert_epoch
) {
  UniversalIndexHolder<>::insertMap(proxy,map_han,insert_epoch);
}

inline VirtualProxyType CollectionManager::makeNewCollectionProxy() {
  auto const& node = theContext()->getNode();
  return VirtualProxyBuilder::createProxy(curIdent_<>++, node, true, true);
}

/*
 * Support of virtual context collection element dynamic insertion
 */

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::insertHandler(InsertMsg<ColT,IndexT>* msg) {
  auto const& epoch = msg->epoch_;
  auto const& g_epoch = msg->g_epoch_;
  theCollection()->insert<ColT,IndexT>(
    msg->proxy_,msg->idx_,msg->construct_node_
  );
  theTerm()->consume(epoch);
  theTerm()->consume(g_epoch);
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::updateInsertEpochHandler(
  UpdateInsertMsg<ColT,IndexT>* msg
) {
  auto const& untyped_proxy = msg->proxy_.getProxy();
  UniversalIndexHolder<>::insertSetEpoch(untyped_proxy,msg->epoch_);

  auto const& root = 0;
  auto nmsg = makeSharedMessage<FinishedUpdateMsg>(untyped_proxy);
  theCollective()->reduce<FinishedUpdateMsg,finishedUpdateHan>(
    root, nmsg, msg->epoch_
  );
}

template <typename>
/*static*/ void CollectionManager::finishedUpdateHan(
  FinishedUpdateMsg* msg
) {
  debug_print(
    vrt_coll, node,
    "finishedUpdateHan: proxy={}, root={}\n",
    msg->proxy_, msg->isRoot()
  );

  if (msg->isRoot()) {
    /*
     *  Trigger any actions that the user may have registered for when insertion
     *  has fully terminated
     */
    return theCollection()->actInsert<>(msg->proxy_);
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::setupNextInsertTrigger(
  VirtualProxyType const& proxy, EpochType const& insert_epoch
) {
  debug_print(
    vrt_coll, node,
    "setupNextInsertTrigger: proxy={}, insert_epoch={}\n",
    proxy, insert_epoch
  );

  auto finished_insert_trigger = [proxy,insert_epoch]{
    debug_print(
      vrt_coll, node,
      "insert finished insert trigger: epoch={}\n",
      insert_epoch
    );
    theCollection()->finishedInsertEpoch<ColT,IndexT>(proxy,insert_epoch);
  };
  auto start_detect = [insert_epoch,finished_insert_trigger]{
    debug_print(
      vrt_coll, node,
      "insert start_detect: epoch={}\n",insert_epoch
    );
    theTerm()->addAction(insert_epoch, finished_insert_trigger);
    theTerm()->activateEpoch(insert_epoch);
  };
  insert_finished_action_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(start_detect)
  );
}

template <typename ColT, typename IndexT>
void CollectionManager::finishedInsertEpoch(
  CollectionProxyWrapType<ColT,IndexT> const& proxy, EpochType const& epoch
) {
  auto const& this_node = theContext()->getNode();
  auto const& untyped_proxy = proxy.getProxy();

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (before) proxy={}, epoch={}\n",
    untyped_proxy, epoch
  );

  /*
   *  Add trigger for the next insertion phase/epoch finishing
   */
  auto const& next_insert_epoch = theTerm()->makeEpochRooted();
  UniversalIndexHolder<>::insertSetEpoch(untyped_proxy,next_insert_epoch);

  auto msg = makeSharedMessage<UpdateInsertMsg<ColT,IndexT>>(
    proxy,next_insert_epoch
  );
  theMsg()->broadcastMsg<
    UpdateInsertMsg<ColT,IndexT>,updateInsertEpochHandler
  >(msg);

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (after broadcast) proxy={}, epoch={}\n",
    untyped_proxy, epoch
  );

  /*
   *  Setup next epoch
   */
  setupNextInsertTrigger<ColT,IndexT>(untyped_proxy,next_insert_epoch);

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (after setup) proxy={}, epoch={}\n",
    untyped_proxy, epoch
  );

  /*
   *  Contribute to reduction for update epoch: this forces the update of the
   *  current insertion epoch to be consistent across the managers *before*
   *  triggering the user's finished epoch handler so that all actions on the
   *  corresponding collection after are related to the new insert epoch
   */
  auto const& root = 0;
  auto nmsg = makeSharedMessage<FinishedUpdateMsg>(untyped_proxy);
  theCollective()->reduce<FinishedUpdateMsg,finishedUpdateHan>(
    root, nmsg, next_insert_epoch
  );

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (after reduce) proxy={}, epoch={}\n",
    untyped_proxy, epoch
  );
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::actInsertHandler(
  ActInsertMsg<ColT,IndexT>* msg
) {
  auto const& untyped_proxy = msg->proxy_.getProxy();
  return theCollection()->actInsert<>(untyped_proxy);
}

template <typename>
void CollectionManager::actInsert(VirtualProxyType const& proxy) {
  debug_print(
    vrt_coll, node,
    "actInsert: proxy={}\n",
    proxy
  );

  auto iter = user_insert_action_.find(proxy);
  if (iter != user_insert_action_.end()) {
    auto action = iter->second;
    user_insert_action_.erase(iter);
    action();
  }
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::doneInsertHandler(
  DoneInsertMsg<ColT,IndexT>* msg
) {
  auto const& node = msg->action_node_;
  auto const& untyped_proxy = msg->proxy_.getProxy();
  if (node != uninitialized_destination) {
    auto send = [untyped_proxy,node]{
      auto msg = makeSharedMessage<ActInsertMsg<ColT,IndexT>>(untyped_proxy);
      theMsg()->sendMsg<
        ActInsertMsg<ColT,IndexT>,actInsertHandler<ColT,IndexT>
      >(node,msg);
    };
    return theCollection()->finishedInserting<ColT,IndexT>(msg->proxy_, send);
  } else {
    return theCollection()->finishedInserting<ColT,IndexT>(msg->proxy_);
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::finishedInserting(
  CollectionProxyWrapType<ColT,IndexT> const& proxy,
  ActionType insert_action
) {
  auto const& this_node = theContext()->getNode();
  auto const& untyped_proxy = proxy.getProxy();
  /*
   *  Register the user's action for when insertion is completed across the
   *  whole system, which termination in the insertion epoch can enforce
   */
  if (insert_action) {
    user_insert_action_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(untyped_proxy),
      std::forward_as_tuple(insert_action)
    );
  }

  auto const& cons_node = VirtualProxyBuilder::getVirtualNode(untyped_proxy);

  debug_print(
    vrt_coll, node,
    "finishedInserting: proxy={}, cons_node={}, this_node={}\n",
    proxy.getProxy(), cons_node, this_node
  );

  if (cons_node == this_node) {
    auto iter = insert_finished_action_.find(untyped_proxy);
    assert(iter != insert_finished_action_.end() && "Insertable should exist");
    auto action = iter->second;
    insert_finished_action_.erase(untyped_proxy);
    action();
  } else {
    auto node = insert_action ? this_node : uninitialized_destination;
    auto msg = makeSharedMessage<DoneInsertMsg<ColT,IndexT>>(proxy,node);
    theMsg()->sendMsg<DoneInsertMsg<ColT,IndexT>,doneInsertHandler<ColT,IndexT>>(
      cons_node,msg
    );
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::insert(
  CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx,
  NodeType const& node
) {
  auto const untyped_proxy = proxy.getProxy();
  auto found_constructed = constructed_.find(untyped_proxy) != constructed_.end();

  debug_print(
    vrt_coll, node,
    "insert: proxy={}, constructed={}\n",
    untyped_proxy, found_constructed
  );

  if (found_constructed) {
    auto col_holder = findColHolder<ColT,IndexT>(untyped_proxy);
    auto max_idx = col_holder->max_idx;
    auto const& this_node = theContext()->getNode();
    auto map_han = UniversalIndexHolder<>::getMap(untyped_proxy);
    auto insert_epoch = UniversalIndexHolder<>::insertGetEpoch(untyped_proxy);
    assert(insert_epoch != no_epoch && "Epoch should be valid");

    bool const& is_functor =
      auto_registry::HandlerManagerType::isHandlerFunctor(map_han);
    auto_registry::AutoActiveMapType fn = nullptr;
    if (is_functor) {
      fn = auto_registry::getAutoHandlerFunctorMap(map_han);
    } else {
      fn = auto_registry::getAutoHandlerMap(map_han);
    }
    auto const& mapped_node = fn(
      reinterpret_cast<vt::index::BaseIndex*>(&idx),
      reinterpret_cast<vt::index::BaseIndex*>(&max_idx),
      theContext()->getNumNodes()
    );

    auto const& has_explicit_node = node != uninitialized_destination;
    auto const& insert_node = has_explicit_node ? node : mapped_node;

    if (insert_node == this_node) {
      auto const& num_elms = max_idx.getSize();
      std::tuple<> tup;

      #if backend_check_enabled(detector)
        auto new_vc = DerefCons::derefTuple<ColT,IndexT,std::tuple<>>(
          num_elms, idx, &tup
        );
      #else
        auto new_vc = CollectionManager::runConstructor<ColT, IndexT>(
          num_elms, idx, &tup, std::make_index_sequence<0>{}
        );
      #endif

      /*
       * Set direct attributes of the newly constructed element directly on
       * the user's class
       */
      CollectionTypeAttorney::setSize(new_vc, num_elms);
      CollectionTypeAttorney::setProxy(new_vc, untyped_proxy);
      CollectionTypeAttorney::setIndex<decltype(new_vc),IndexT>(new_vc, idx);

      theCollection()->insertCollectionElement<ColT, IndexT>(
        std::move(new_vc), idx, max_idx, map_han, untyped_proxy, mapped_node
      );
    } else {
      auto const& global_epoch = theMsg()->getGlobalEpoch();
      auto msg = makeSharedMessage<InsertMsg<ColT,IndexT>>(
        proxy,max_idx,idx,insert_node,mapped_node,insert_epoch,global_epoch
      );
      theTerm()->produce(insert_epoch);
      theTerm()->produce(global_epoch);
      theMsg()->sendMsg<InsertMsg<ColT,IndexT>,insertHandler<ColT,IndexT>>(
        insert_node,msg
      );
    }
  } else {
    auto insert_epoch = UniversalIndexHolder<>::insertGetEpoch(untyped_proxy);
    auto iter = buffered_bcasts_.find(untyped_proxy);
    if (iter == buffered_bcasts_.end()) {
      buffered_bcasts_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(untyped_proxy),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_bcasts_.find(untyped_proxy);
    }
    assert(iter != buffered_bcasts_.end() and "Must exist");

    debug_print(
      vrt_coll, node,
      "pushing dynamic insertion into buffered sends: {}\n",
      untyped_proxy
    );

    auto const& global_epoch = theMsg()->getGlobalEpoch();
    theTerm()->produce(global_epoch);
    theTerm()->produce(insert_epoch);

    debug_print(
      vrt_coll, node,
      "insert: proxy={}, buffering\n", untyped_proxy
    );
    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      debug_print(
        vrt_coll, node,
        "insert: proxy={}, running buffered\n", untyped_proxy
      );
      theCollection()->insert<ColT>(proxy,idx,node);
      theTerm()->consume(insert_epoch);
      theTerm()->consume(global_epoch);
    });
  }
}

/*
 * Support of virtual context collection element migration
 */

template <typename ColT>
MigrateStatus CollectionManager::migrate(
  VrtElmProxy<ColT, typename ColT::IndexType> proxy, NodeType const& dest
) {
  using IndexT = typename ColT::IndexType;
  auto const& col_proxy = proxy.getCollectionProxy();
  auto const& elm_proxy = proxy.getElementProxy();
  auto const& idx = elm_proxy.getIndex();
  return migrateOut<ColT,IndexT>(col_proxy, idx, dest);
}

template <typename ColT, typename IndexT>
MigrateStatus CollectionManager::migrateOut(
  VirtualProxyType const& col_proxy, IndexT const& idx, NodeType const& dest
) {
 auto const& this_node = theContext()->getNode();

 debug_print(
   vrt_coll, node,
   "migrateOut: col_proxy={}, this_node={}, dest={}, "
   "idx={}\n",
   col_proxy, this_node, dest, print_index(idx)
 );

 if (this_node != dest) {
   auto const& proxy = CollectionIndexProxy<ColT, IndexT>(col_proxy).operator()(
     idx
   );
   auto elm_holder = findElmHolder<ColT, IndexT>(col_proxy);
   assert(
     elm_holder != nullptr && "Element must be registered here"
   );

   #if backend_check_enabled(runtime_checks)
   {
     bool const& exists = elm_holder->exists(idx);
     assert(
       exists && "Local element must exist here for migration to occur"
     );
   }
   #endif

   bool const& exists = elm_holder->exists(idx);
   if (!exists) {
     return MigrateStatus::ElementNotLocal;
   }

   debug_print(
     vrt_coll, node,
     "migrateOut: (before remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   auto& coll_elm_info = elm_holder->lookup(idx);
   auto map_fn = coll_elm_info.map_fn;
   auto range = coll_elm_info.max_idx;
   auto col_unique_ptr = elm_holder->remove(idx);
   auto& typed_col_ref = *static_cast<ColT*>(col_unique_ptr.get());

   debug_print(
     vrt_coll, node,
     "migrateOut: (after remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   /*
    * Invoke the virtual prelude migrate out function
    */
   col_unique_ptr->preMigrateOut();

   debug_print(
     vrt_coll, node,
     "migrateOut: col_proxy={}, idx={}, dest={}: serializing collection elm\n",
     col_proxy, print_index(idx), dest
   );

   using MigrateMsgType = MigrateMsg<ColT, IndexT>;

   auto msg = makeSharedMessage<MigrateMsgType>(
     proxy, this_node, dest, map_fn, range
   );

   SerialByteType* buf = nullptr;
   SizeType buf_size = 0;
   ::serialization::interface::serialize<ColT>(
     typed_col_ref,
     [&buf,&buf_size](SizeType size) -> SerialByteType* {
       buf_size = size;
       buf = static_cast<SerialByteType*>(std::malloc(buf_size));
       return buf;
     }
   );
   msg->setPut(buf, buf_size);

   // @todo: action here to free put buffer
   theMsg()->sendMsg<
     MigrateMsgType, MigrateHandlers::migrateInHandler<ColT, IndexT>
   >(dest, msg, [msg]{});

   theLocMan()->getCollectionLM<ColT, IndexT>(col_proxy)->entityMigrated(
     proxy, dest
   );

   /*
    * Invoke the virtual epilog migrate out function
    */
   col_unique_ptr->epiMigrateOut();

   debug_print(
     vrt_coll, node,
     "migrateOut: col_proxy={}, idx={}, dest={}: invoking destroy()\n",
     col_proxy, print_index(idx), dest
   );

   /*
    * Invoke the virtual destroy function and then null std::unique_ptr<ColT>,
    * which should cause the destructor to fire
    */
   col_unique_ptr->destroy();
   col_unique_ptr = nullptr;

   return MigrateStatus::MigratedToRemote;
 } else {
   #if backend_check_enabled(runtime_checks)
     assert(
       false && "Migration should only be called when to_node is != this_node"
     );
   #else
     // Do nothing
   #endif
   return MigrateStatus::NoMigrationNecessary;
 }
}

template <typename ColT, typename IndexT>
MigrateStatus CollectionManager::migrateIn(
  VirtualProxyType const& proxy, IndexT const& idx, NodeType const& from,
  VirtualPtrType<ColT, IndexT> vrt_elm_ptr, IndexT const& max,
  HandlerType const& map_han
) {
  debug_print(
    vrt_coll, node,
    "CollectionManager::migrateIn: proxy={}, idx={}, from={}, ptr={}\n",
    proxy, print_index(idx), from, print_ptr(vrt_elm_ptr.get())
  );

  auto vc_raw_ptr = vrt_elm_ptr.get();

  /*
   * Invoke the virtual prelude migrate-in function
   */
  vc_raw_ptr->preMigrateIn();

  auto const& elm_proxy = CollectionIndexProxy<ColT, IndexT>(proxy).operator()(
    idx
  );

  auto const& home_node = uninitialized_destination;
  auto const& inserted = insertCollectionElement<ColT, IndexT>(
    std::move(vrt_elm_ptr), idx, max, map_han, proxy, home_node, true, from
  );

  /*
   * Invoke the virtual epilog migrate-in function
   */
  vc_raw_ptr->epiMigrateIn();

  if (inserted) {
    return MigrateStatus::MigrateInLocal;
  } else {
    return MigrateStatus::DestroyedDuringMigrated;
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::destroy(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  using DestroyMsgType = DestroyMsg<ColT, IndexT>;
  auto const& this_node = theContext()->getNode();
  auto msg = makeSharedMessage<DestroyMsgType>(proxy, this_node);
  theMsg()->broadcastMsg<DestroyMsgType, DestroyHandlers::destroyNow>(msg);
  auto msg_this = makeSharedMessage<DestroyMsgType>(proxy, this_node);
  messageRef(msg_this);
  DestroyHandlers::destroyNow(msg_this);
  messageDeref(msg_this);
}

template <typename ColT, typename IndexT>
void CollectionManager::incomingDestroy(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  destroyMatching<ColT,IndexT>(proxy);
}

template <typename ColT, typename IndexT>
void CollectionManager::destroyMatching(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  UniversalIndexHolder<>::destroyCollection(proxy.getProxy());
  auto elm_holder = findElmHolder<ColT,IndexT>(proxy.getProxy());
  elm_holder->destroyAll();
}

template <typename ColT, typename IndexT>
CollectionHolder<ColT, IndexT>* CollectionManager::findColHolder(
  VirtualProxyType const& proxy
) {
  #pragma sst global proxy_container_
  auto& holder_container = EntireHolder<ColT, IndexT>::proxy_container_;
  auto holder_iter = holder_container.find(proxy);
  auto const& found_holder = holder_iter != holder_container.end();
  if (found_holder) {
    return holder_iter->second.get();
  } else {
    return nullptr;
  }
}

template <typename ColT, typename IndexT>
Holder<ColT, IndexT>* CollectionManager::findElmHolder(
  VirtualProxyType const& proxy
) {
  auto ret = findColHolder<ColT, IndexT>(proxy);
  if (ret != nullptr) {
    return &ret->holder_;
  } else {
    return nullptr;
  }
}

template <typename>
std::size_t CollectionManager::numCollections() {
  return UniversalIndexHolder<>::getNumCollections();
}

template <typename>
std::size_t CollectionManager::numReadyCollections() {
  return UniversalIndexHolder<>::getNumReadyCollections();
}

template <typename>
void CollectionManager::resetReadyPhase() {
  UniversalIndexHolder<>::resetPhase();
}

template <typename>
bool CollectionManager::readyNextPhase() {
  auto const ready = UniversalIndexHolder<>::readyNextPhase();
  return ready;
}

template <typename>
void CollectionManager::makeCollectionReady(VirtualProxyType const proxy) {
  UniversalIndexHolder<>::makeCollectionReady(proxy);
}

template <typename ColT>
void CollectionManager::nextPhase(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& proxy,
  PhaseType const& cur_phase, ActionFinishedLBType continuation
) {
  using namespace balance;
  using MsgType = PhaseMsg<ColT>;
  auto msg = makeSharedMessage<MsgType>(cur_phase, proxy);
  auto const& instrument = false;

  debug_print(
    vrt_coll, node,
    "nextPhase: broadcasting: cur_phase={}\n",
    cur_phase
  );

  if (continuation != nullptr) {
    theTerm()->produce(term::any_epoch_sentinel);
    lb_continuations_.push_back(continuation);

    auto const& untyped_proxy = proxy.getProxy();
    auto iter = lb_no_elm_.find(untyped_proxy);
    if (iter ==lb_no_elm_.end()) {
      lb_no_elm_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(untyped_proxy),
        std::forward_as_tuple([this,untyped_proxy]{
          auto elm_holder =
            findElmHolder<ColT,typename ColT::IndexType>(untyped_proxy);
          auto const& num_elms = elm_holder->numElements();
          // this collection manager does not participate in reduction
          if (num_elms == 0) {
            /*
             * @todo: Implement child elision in reduction tree and up
             * propagation
             */
          }
        })
      );
    }
  }

  backend_enable_if(
    lblite, {
      msg->setLBLiteInstrument(instrument);
      debug_print(
        vrt_coll, node,
        "nextPhase: broadcasting: instrument={}, cur_phase={}\n",
        msg->lbLiteInstrument(), cur_phase
      );
    }
  );
  theCollection()->broadcastMsg<MsgType,ElementStats::syncNextPhase<ColT>>(
    proxy, msg, nullptr, instrument
  );
}

template <typename ColT>
void CollectionManager::computeStats(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& proxy,
  PhaseType const& cur_phase
) {
  using namespace balance;
  using MsgType = PhaseMsg<ColT>;
  auto msg = makeSharedMessage<MsgType>(cur_phase,proxy);
  auto const& instrument = false;

  debug_print(
    vrt_coll, node,
    "computeStats: broadcasting: cur_phase={}\n",
    cur_phase
  );

  theCollection()->broadcastMsg<MsgType,ElementStats::computeStats<ColT>>(
    proxy, msg, nullptr, instrument
  );
}

template <typename always_void>
void CollectionManager::checkReduceNoElements() {
  // @todo
}

template <typename always_void>
/*static*/ void CollectionManager::releaseLBPhase(CollectionPhaseMsg* msg) {
  theCollection()->releaseLBContinuation();
}

template <typename>
void CollectionManager::releaseLBContinuation() {
  UniversalIndexHolder<>::resetPhase();
  if (lb_continuations_.size() > 0) {
    auto continuations = lb_continuations_;
    lb_continuations_.clear();
    for (auto&& elm : continuations) {
      theTerm()->consume(term::any_epoch_sentinel);
      elm();
    }
  }
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H*/
