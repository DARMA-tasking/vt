
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H

#include "config.h"
#include "vrt/collection/holders/col_holder.h"
#include "epoch/epoch.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename=void>
struct UniversalIndexHolder {
  static void destroyAllLive();
  static void destroyCollection(VirtualProxyType const proxy);
  static bool readyNextPhase();
  static void makeCollectionReady(VirtualProxyType const proxy);
  static void resetPhase();
  static std::size_t getNumCollections();
  static std::size_t getNumReadyCollections();
  static void insertMap(VirtualProxyType const proxy, HandlerType const& han);
  static HandlerType getMap(VirtualProxyType const proxy);
public:
  static std::unordered_set<VirtualProxyType> ready_collections_;
  static std::unordered_map<
    VirtualProxyType,std::shared_ptr<BaseHolder>
  > live_collections_;
  static std::unordered_map<VirtualProxyType,HandlerType> live_collections_map_;
private:
  static std::size_t num_collections_phase_;
};

template <typename ColT, typename IndexT>
struct EntireHolder {
  using InnerHolder = CollectionHolder<ColT, IndexT>;
  using InnerHolderPtr = std::shared_ptr<InnerHolder>;
  using ProxyContainerType = std::unordered_map<
    VirtualProxyType, InnerHolderPtr
  >;
  static void insert(VirtualProxyType const& proxy, InnerHolderPtr ptr);
  static ProxyContainerType proxy_container_;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/holders/entire_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H*/
