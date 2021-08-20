/*
//@HEADER
// *****************************************************************************
//
//                      stats_driven_collection_mapper.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_MAPPER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_MAPPER_H

#include "vt/config.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/topos/mapping/base_mapper_object.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct StatsDrivenCollectionMapper
 *
 * \brief Tracks the layout of a collection and the mapping between vt
 * index and element id.
 */
template <typename IndexType>
struct StatsDrivenCollectionMapper : vt::mapping::BaseMapper<IndexType> {
  using ThisType = StatsDrivenCollectionMapper<IndexType>;
  using IndexVec = std::vector<uint64_t>;
  using ElmIDType = ElementIDType;

  StatsDrivenCollectionMapper() = default;

  NodeType map(IndexType* idx, int ndim, NodeType num_nodes) override {
    // correct operation here requires that the home rank specifically
    // know that it maps locally
    auto it = rank_mapping_.find(*idx);
    if (it != rank_mapping_.end()) {
      vt_debug_print(
        normal, replay,
        "StatsDrivenCollectionMapper: index {} maps to rank {}\n",
        *idx, it->second
      );
      return it->second;
    }
    return uninitialized_destination;
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | rank_mapping_
      | elm_to_index_mapping_;
  }

  void addCollectionMapping(IndexType idx, NodeType home);

  void addElmToIndexMapping(ElmIDType elm_id, IndexType index);

  void addElmToIndexMapping(ElmIDType elm_id, IndexVec idx_vec);

  IndexType getIndexFromElm(ElmIDType elm_id);

private:
  std::map<IndexType, int /*mpi_rank*/> rank_mapping_ = {};

  /// \brief Mapping from element ids to vt indices
  std::unordered_map<ElmIDType, IndexType> elm_to_index_mapping_ = {};
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_MAPPER_H*/
