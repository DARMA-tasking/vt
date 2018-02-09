
#if !defined INCLUDED_VRT_COLLECTION_DEFAULTS_DEFAULT_MAP_H
#define INCLUDED_VRT_COLLECTION_DEFAULTS_DEFAULT_MAP_H

#include "config.h"
#include "topos/mapping/mapping_headers.h"
#include "topos/mapping/dense/dense.h"

#include <tuple>
#include <type_traits>

namespace vt { namespace vrt { namespace collection {

template <typename CollectionT>
struct DefaultMapBase {
  using IndexType        = typename CollectionT::IndexType;
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
    std::is_same<typename CollectionT::IndexType, ::vt::index::Index1D>::value
  >
> : DefaultMapBase<CollectionT>
{
  using BlockMapType     = ::vt::mapping::dense1DBlkMapFn;
  using RRMapType        = ::vt::mapping::dense1DRRMapFn;
  using DefaultMapType   = ::vt::mapping::dense1DMapFn;
  using MapType          = DefaultMapType;
};

/*
 * Default mappings for Index2D: RR, Block, etc.
 */

template <typename CollectionT>
struct DefaultMap<
  CollectionT,
  typename std::enable_if_t<
    std::is_same<typename CollectionT::IndexType, ::vt::index::Index2D>::value
  >
> : DefaultMapBase<CollectionT>
{
  using BlockMapType     = ::vt::mapping::dense2DBlkMapFn;
  using RRMapType        = ::vt::mapping::dense2DRRMapFn;
  using DefaultMapType   = ::vt::mapping::dense2DMapFn;
  using MapType          = DefaultMapType;
};

/*
 * Default mappings for Index3D: RR, Block, etc.
 */

template <typename CollectionT>
struct DefaultMap<
  CollectionT,
  typename std::enable_if_t<
    std::is_same<typename CollectionT::IndexType, ::vt::index::Index3D>::value
  >
> : DefaultMapBase<CollectionT>
{
  using BlockMapType     = ::vt::mapping::dense3DBlkMapFn;
  using RRMapType        = ::vt::mapping::dense3DRRMapFn;
  using DefaultMapType   = ::vt::mapping::dense3DMapFn;
  using MapType          = DefaultMapType;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DEFAULTS_DEFAULT_MAP_H*/
