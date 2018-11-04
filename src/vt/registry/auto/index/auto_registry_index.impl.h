
#if !defined INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_IMPL_H
#define INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/index/auto_registry_index.h"
#include "vt/registry/auto/index/auto_registry_index_reg.h"

#include <cstdlib>

namespace vt { namespace auto_registry {

inline std::size_t getMaxIndexSize() {
  return MaxIndexHolder<>::index_max_size;
}

template <typename IndexT>
inline AutoHandlerType makeAutoIndex() {
  auto const unique_idx = IndexHolder<IndexT>::idx;
  return unique_idx;
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_IMPL_H*/
