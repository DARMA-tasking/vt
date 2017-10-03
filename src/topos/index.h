
#if ! defined __RUNTIME_TRANSPORT_INDEX__
#define __RUNTIME_TRANSPORT_INDEX__

#include "config.h"
#include "topos/example_index.h"
#include "topos/dense_index_array.h"

#if backend_check_enabled(detector)
  #include "index/index_traits.h"
#endif

#include <cstdint>

namespace vt { namespace index {

using Index1D = DenseIndexArray<int32_t, 1>;
using Index2D = DenseIndexArray<int32_t, 2>;
using Index3D = DenseIndexArray<int32_t, 3>;

#if backend_check_enabled(detector)
  static_assert(
    vt::index::IndexTraits<Index1D>::is_index, "Index1D must follow index concept"
  );
  static_assert(
    vt::index::IndexTraits<Index2D>::is_index, "Index2D must be an index"
  );
  static_assert(
    vt::index::IndexTraits<Index3D>::is_index, "Index3D must be an index"
  );
#endif

}} // end namespace vt::index

namespace vt {

template <typename IndexType, index::NumDimensionsType ndim>
using DenseIndex = index::DenseIndexArray<IndexType, ndim>;

using Index1D = index::Index1D;
using Index2D = index::Index2D;
using Index3D = index::Index3D;

}  // end namespace vt

#endif /*__RUNTIME_TRANSPORT_INDEX__*/
