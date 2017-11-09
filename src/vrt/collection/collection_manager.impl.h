
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_IMPL_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_IMPL_H

#include "config.h"
#include "context/context.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/collection_manager.h"
#include "vrt/collection/collection_create_msg.h"
#include "vrt/collection/collection_info.h"
#include "registry/auto_registry_map.h"
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
          std::move(new_vc), cur_idx, new_proxy
        );
      }
    });
  } else {
    // just wait and register the proxy
  }
}

template <typename IndexT>
void CollectionManager::insertCollectionElement(
  VirtualPtrType<IndexT> vc, IndexT const& idx, VirtualProxyType const &proxy
) {
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
      std::forward_as_tuple(std::move(vc))
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
