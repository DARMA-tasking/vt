
#if !defined INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_H
#define INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"

namespace vt { namespace auto_registry {

std::size_t getMaxIndexSize();

template <typename IndexT>
AutoHandlerType makeAutoIndex();

}} // end namespace vt::auto_registry

#include "vt/registry/auto/index/auto_registry_index.impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_H*/
