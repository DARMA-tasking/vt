
#if ! defined __RUNTIME_TRANSPORT_MAPPING__
#define __RUNTIME_TRANSPORT_MAPPING__

#include "config.h"
#include "index.h"

#include <functional>

namespace vt { namespace mapping {

template <typename PhysicalType, typename IndexType>
using MapType = std::function<PhysicalType(IndexType const& idx)>;

template <typename IndexType>
using NodeMapType = MapType<NodeType, IndexType>;

template <typename IndexType>
using CoreMapType = MapType<CoreType, IndexType>;


}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_MAPPING__*/
