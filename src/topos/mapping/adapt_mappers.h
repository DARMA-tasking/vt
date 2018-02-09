
#if !defined INCLUDED_TOPOS_MAPPING_ADAPT_MAPPERS_H
#define INCLUDED_TOPOS_MAPPING_ADAPT_MAPPERS_H

#include "config.h"
#include "registry/auto_registry_general.h"

namespace vt { namespace mapping {

template <typename IndexT>
using MapAdapter = PhysicalResourceType(IndexT*, IndexT*, PhysicalResourceType);

template <typename F, F* f>
using FunctorAdapt = ::vt::auto_registry::FunctorAdapter<F,f>;

}}  // end namespace vt::mapping

#endif /*INCLUDED_TOPOS_MAPPING_ADAPT_MAPPERS_H*/
