
#if !defined INCLUDED_VRT_COLLECTION_DEFAULTS_DEFAULT_MAP_H
#define INCLUDED_VRT_COLLECTION_DEFAULTS_DEFAULT_MAP_H

#include "vt/config.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/topos/mapping/dense/dense.h"

#include <tuple>
#include <type_traits>

namespace vt { namespace vrt { namespace collection {

template <typename CollectionT>
struct DefaultMapBase {
  using IndexType        = typename CollectionT::IndexType;
  using BaseType         = typename IndexType::DenseIndexType;
  using IndexPtrType     = IndexType*;
  using MapParamPackType = std::tuple<IndexPtrType,IndexPtrType,NodeType>;
};

template <typename CollectionT, typename Enable=void>
struct DefaultMap;

/*
 * Default mappings for Index1D: RR, Block, etc.
 */

template <typename CollectionT>
struct DefaultMap<
  CollectionT,
  typename std::enable_if_t<
    std::is_same<
      typename CollectionT::IndexType,
      typename ::vt::index::Index1D<
        typename CollectionT::IndexType::DenseIndexType
      >
    >::value
  >
> : DefaultMapBase<CollectionT>
{
  using BaseType         = typename CollectionT::IndexType::DenseIndexType;
  using BlockMapType     = ::vt::mapping::dense1DBlkMapFn<BaseType>;
  using RRMapType        = ::vt::mapping::dense1DRRMapFn<BaseType>;
  using DefaultMapType   = ::vt::mapping::dense1DMapFn<BaseType>;
  using MapType          = DefaultMapType;
};

/*
 * Default mappings for Index2D: RR, Block, etc.
 */

template <typename CollectionT>
struct DefaultMap<
  CollectionT,
  typename std::enable_if_t<
    std::is_same<
      typename CollectionT::IndexType,
      typename ::vt::index::Index2D<
        typename CollectionT::IndexType::DenseIndexType
      >
    >::value
  >
> : DefaultMapBase<CollectionT>
{
  using BaseType         = typename CollectionT::IndexType::DenseIndexType;
  using BlockMapType     = ::vt::mapping::dense2DBlkMapFn<BaseType>;
  using RRMapType        = ::vt::mapping::dense2DRRMapFn<BaseType>;
  using DefaultMapType   = ::vt::mapping::dense2DMapFn<BaseType>;
  using MapType          = DefaultMapType;
};

/*
 * Default mappings for Index3D: RR, Block, etc.
 */

template <typename CollectionT>
struct DefaultMap<
  CollectionT,
  typename std::enable_if_t<
    std::is_same<
      typename CollectionT::IndexType,
      typename ::vt::index::Index3D<
        typename CollectionT::IndexType::DenseIndexType
      >
    >::value
  >
> : DefaultMapBase<CollectionT>
{
  using BaseType         = typename CollectionT::IndexType::DenseIndexType;
  using BlockMapType     = ::vt::mapping::dense3DBlkMapFn<BaseType>;
  using RRMapType        = ::vt::mapping::dense3DRRMapFn<BaseType>;
  using DefaultMapType   = ::vt::mapping::dense3DMapFn<BaseType>;
  using MapType          = DefaultMapType;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DEFAULTS_DEFAULT_MAP_H*/
