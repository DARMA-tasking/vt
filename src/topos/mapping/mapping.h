
#if !defined INCLUDED_TOPOS_MAPPING
#define INCLUDED_TOPOS_MAPPING

#include <functional>

#include "config.h"
#include "topos/index/index.h"
#include "mapping_function.h"
#include "auto_registry_map.h"

namespace vt { namespace mapping {

// General map function type: contains only the index and number of nodes
template <typename PhysicalType, typename IndexType>
using MapType = PhysicalType(*)(IndexType*, NodeType);

template <typename IndexType>
using NodeMapType = MapType<NodeType, IndexType>;
template <typename IndexType>
using CoreMapType = MapType<CoreType, IndexType>;


// Dense index map function type: contains index and size of dense region
template <typename PhysicalType, typename IndexType>
using DenseMapType = PhysicalType(*)(IndexType*, IndexType*, NodeType);

template <typename IndexType>
using DenseNodeMapType = DenseMapType<NodeType, IndexType>;
template <typename IndexType>
using DenseCoreMapType = DenseMapType<CoreType, IndexType>;

}}  // end namespace vt::location

#endif  /*INCLUDED_TOPOS_MAPPING*/
