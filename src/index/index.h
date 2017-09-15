
#if ! defined __RUNTIME_TRANSPORT_INDEX__
#define __RUNTIME_TRANSPORT_INDEX__

#include "config.h"
#include "dense_index_array.h"

#include <cstdint>

namespace vt { namespace index {

using Index1D = DenseIndexArray<int32_t, 1>;
using Index2D = DenseIndexArray<int32_t, 2>;
using Index3D = DenseIndexArray<int32_t, 3>;

}} // end namespace vt::index

namespace vt {

template <typename IndexType, index::NumDimensionsType ndim>
using DenseIndex = index::DenseIndexArray<IndexType, ndim>;

using Index1D = index::Index1D;
using Index2D = index::Index2D;
using Index3D = index::Index3D;

}

#endif /*__RUNTIME_TRANSPORT_INDEX__*/
