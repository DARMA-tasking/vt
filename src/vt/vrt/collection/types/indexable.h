/*
//@HEADER
// *****************************************************************************
//
//                                 indexable.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/types/type_attorney.h"
#include "vt/vrt/collection/types/migrate_hooks.h"
#include "vt/vrt/collection/types/migratable.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Indexable : Migratable {
  using IndexType = IndexT;
  using ReduceStampType = collective::reduce::detail::ReduceStamp;
  using ReduceSeqStampType = collective::reduce::detail::StrongSeq;

  explicit Indexable(IndexT&& in_index);
  Indexable() = default;

  IndexT const& getIndex() const;

protected:
  template <typename Serializer>
  void serialize(Serializer& s);

private:
  friend struct CollectionTypeAttorney;
  friend struct CollectionManager;

  void setIndex(IndexT const& in_index);

private:
  // The index stored with the collection element
  IndexT index_;
  // This field stores whether the `index_` has been properly set: if the
  // constructor overload has no index, it will not be set until the its set
  // through the `CollectionTypeAttorney`
  bool set_index_ = false;

public:
  /**
   * \brief Get the next reduce stamp and increment
   *
   * \return the reduce stamp
   */
  ReduceStampType getNextStamp();

  /**
   * \brief Zero out the reduce stamp
   */
  void zeroReduceStamp();

protected:
  ReduceSeqStampType reduce_stamp_ = ReduceSeqStampType{1};
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_H*/
