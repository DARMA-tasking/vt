/*
//@HEADER
// ************************************************************************
//
//                          reducable.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_H
#define INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/activefn/activefn.h"
#include "vt/pipe/pipe_callback_only.h"
#include "vt/collective/reduce/operators/functors/none_op.h"

#include <functional>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Reducable : BaseProxyT {
  using ReduceIdxFuncType = std::function<bool(IndexT const&)>;

  Reducable() = default;
  Reducable(Reducable const&) = default;
  Reducable(Reducable&&) = default;
  explicit Reducable(VirtualProxyType const in_proxy);
  Reducable& operator=(Reducable const&) = default;


  template <typename OpT = collective::None, typename MsgT>
  EpochType reduce(
    MsgT *const msg, Callback<MsgT> cb, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag
  ) const;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduce(
    MsgT *const msg, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag,
    NodeType const& node = uninitialized_destination
  ) const;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceExpr(
    MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag,
    NodeType const& node = uninitialized_destination
  ) const;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduce(
    MsgT *const msg, EpochType const& epoch, TagType const& tag,
    IndexT const& idx
  ) const;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceExpr(
    MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch,
    TagType const& tag,
    IndexT const& idx
  ) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_H*/
