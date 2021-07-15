/*
//@HEADER
// *****************************************************************************
//
//                                    base.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_BASE_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_BASE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/types/base.fwd.h"
#include "vt/vrt/collection/types/insertable.h"
#include "vt/vrt/collection/types/indexable.h"
#include "vt/vrt/collection/types/untyped.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/collective/reduce/scoping/strong_types.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct CollectionBase : Indexable<IndexT> {
  using ProxyType = VirtualElmProxyType<ColT, IndexT>;
  using CollectionProxyType = CollectionProxy<ColT, IndexT>;
  using IndexType = IndexT;
  using ReduceStampType = collective::reduce::detail::ReduceStamp;
  using ReduceSeqStampType = collective::reduce::detail::StrongSeq;

  CollectionBase() = default;
  CollectionBase(
    bool const static_size, bool const elms_fixed,
    VirtualElmCountType const num = -1
  );

  virtual ~CollectionBase();

  ProxyType getElementProxy(IndexT const& idx) const;
  CollectionProxyType getCollectionProxy() const;

  bool isStatic() const;

  static bool isStaticSized();

  void setSize(VirtualElmCountType const& elms);

  // Should be implemented in derived class (non-virtual)
  VirtualElmCountType getSize() const;

  virtual void migrate(NodeType const& node) override;

  template <typename Serializer>
  void serialize(Serializer& s);

  friend struct CollectionManager;

  /**
   * \brief Get the next reduce stamp and increment
   *
   * \return the reduce stamp
   */
  ReduceStampType getStampInc();

protected:
  VirtualElmCountType numElems_ = no_elms;
  EpochType cur_bcast_epoch_ = 0;
  bool hasStaticSize_ = true;
  bool elmsFixedAtCreation_ = true;
  ReduceSeqStampType reduce_stamp_ = ReduceSeqStampType{1};
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_BASE_H*/
