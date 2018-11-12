
#if !defined INCLUDED_TOPOS_INDEX
#define INCLUDED_TOPOS_INDEX

#include <cstdint>

#include "vt/config.h"
#include "vt/topos/index/index_example.h"
#include "vt/topos/index/dense/dense_array.h"

#if backend_check_enabled(detector)
  #include "vt/topos/index/traits/traits.h"
#endif

namespace vt { namespace index {

using IdxBase = int32_t;

template <typename T = IdxBase> using Index1D  = DenseIndexArray<T, 1>;
template <typename T = IdxBase> using Index2D  = DenseIndexArray<T, 2>;
template <typename T = IdxBase> using Index3D  = DenseIndexArray<T, 3>;
template <typename T, int8_t N> using IdxType = DenseIndexArray<T, N>;

#if backend_check_enabled(detector)
  static_assert(IndexTraits<Index1D<IdxBase>>::is_index, "Does not conform");
  static_assert(IndexTraits<Index2D<IdxBase>>::is_index, "Does not conform");
  static_assert(IndexTraits<Index3D<IdxBase>>::is_index, "Does not conform");
#endif

}}  // end namespace vt::index

namespace vt {

template <typename IndexType, index::NumDimensionsType ndim>
using DenseIndex = index::DenseIndexArray<IndexType, ndim>;
using IdxBase    = index::IdxBase;

using Index1D  = index::Index1D<index::IdxBase>;
using Index2D  = index::Index2D<index::IdxBase>;
using Index3D  = index::Index3D<index::IdxBase>;

template <typename T, int8_t N> using IdxType   = index::IdxType<T, N>;
template <typename T>           using IdxType1D = index::Index1D<T>;
template <typename T>           using IdxType2D = index::Index2D<T>;
template <typename T>           using IdxType3D = index::Index3D<T>;

}  // end namespace vt

#include "vt/topos/index/printer/print_index.h"

#endif  /*INCLUDED_TOPOS_INDEX*/
