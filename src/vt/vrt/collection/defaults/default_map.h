/*
//@HEADER
// *****************************************************************************
//
//                                default_map.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

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
