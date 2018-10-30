
#if !defined INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_REG_H
#define INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_REG_H

#include "config.h"
#include "registry/auto/auto_registry_common.h"
#include "registry/auto/auto_registry_general.h"
#include "registry/registry.h"

namespace vt { namespace auto_registry {

template <typename=void>
struct MaxIndexHolder {
  static std::size_t index_max_size;
};

template <typename T>
/*static*/ std::size_t MaxIndexHolder<T>::index_max_size = 0;

template <typename IndexT>
struct RegistrarIndex {
  AutoHandlerType index;

  RegistrarIndex();
};

template <typename IndexT>
RegistrarIndex<IndexT>::RegistrarIndex() {
  auto& reg = getAutoRegistryGen<AutoActiveIndexContainerType>();
  index = reg.size();
  MaxIndexHolder<>::index_max_size = std::max(
    MaxIndexHolder<>::index_max_size,
    sizeof(IndexT)
  );
}

template <typename IndexT>
struct RegistrarWrapperIndex {
  RegistrarIndex<IndexT> registrar;
};

template <typename IndexT>
struct IndexHolder {
  static AutoHandlerType const idx;
};

template <typename IndexT>
AutoHandlerType registerIndex() {
  return RegistrarWrapperIndex<IndexT>().registrar.index;
}

template <typename IndexT>
AutoHandlerType const IndexHolder<IndexT>::idx = registerIndex<IndexT>();

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_REG_H*/
