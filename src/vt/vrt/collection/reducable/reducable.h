/*
//@HEADER
// *****************************************************************************
//
//                                 reducable.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_REDUCABLE_REDUCABLE_H
#define INCLUDED_VT_VRT_COLLECTION_REDUCABLE_REDUCABLE_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/activefn/activefn.h"
#include "vt/pipe/pipe_callback_only.h"
#include "vt/collective/reduce/operators/functors/none_op.h"
#include "vt/collective/reduce/operators/callback_op.h"
#include "vt/collective/reduce/reduce_scope.h"

#include <functional>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Reducable : BaseProxyT {
  using ReduceStamp = collective::reduce::ReduceStamp;
  using ReduceIdxFuncType = std::function<bool(IndexT const&)>;

  Reducable() = default;
  Reducable(Reducable const&) = default;
  Reducable(Reducable&&) = default;
  explicit Reducable(VirtualProxyType const in_proxy);
  Reducable& operator=(Reducable const&) = default;

  /**
   * \brief All-reduce back to this collection. Performs a reduction using
   * operator `Op` followed by a broadcast to `f` with the result.
   *
   * \param[in] args the arguments to reduce. \note The last argument optionally
   * may be a `ReduceStamp`.
   *
   * \return a pending send
   */
  template <
    auto f,
    template <typename Arg> class Op = collective::NoneOp,
    typename... Args
  >
  messaging::PendingSend allreduce(
    Args&&... args
  ) const;

  /**
   * \brief Reduce back to a point target. Performs a reduction using operator
   * `Op` followed by a send to `f` with the result.
   *
   * \param[in] args the arguments to reduce. \note The last argument optionally
   * may be a `ReduceStamp`.
   *
   * \return a pending send
   */
  template <
    auto f,
    template <typename Arg> class Op = collective::NoneOp,
    typename Target,
    typename... Args
  >
  messaging::PendingSend reduce(
    Target target,
    Args&&... args
  ) const;

  /**
   * \brief Reduce back to an arbitrary callback. Performs a reduction using
   * operator `Op` and then delivers the result to the callback `cb`.
   *
   * \param[in] cb the callback to trigger with the reduction result
   * \param[in] args the arguments to reduce. \note The last argument optionally
   * may be a `ReduceStamp`.
   *
   * \return a pending send
   */
  template <
    template <typename Arg> class Op = collective::NoneOp,
    typename... CBArgs,
    typename... Args
  >
  messaging::PendingSend reduce(
    vt::Callback<CBArgs...> cb,
    Args&&... args
  ) const;

  template <
    typename OpT = collective::None,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f
  >
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduce(
    MsgT *const msg, Callback<MsgT> cb, ReduceStamp stamp = ReduceStamp{}
  ) const;

  template <
    typename OpT = collective::None,
    typename MsgT
  >
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduce(
    MsgT *const msg, Callback<MsgT> cb, ReduceStamp stamp = ReduceStamp{}
  ) const
  {
    return reduce<
      OpT,
      MsgT,
      &MsgT::template msgHandler<
        MsgT, OpT, collective::reduce::operators::ReduceCallback<MsgT>
        >
      >(msg, cb, stamp);
  }

  template <
    typename OpT,
    typename FunctorT,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f
  >
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduce(MsgT *const msg, ReduceStamp stamp = ReduceStamp{}) const;

  template <
    typename OpT,
    typename FunctorT,
    typename MsgT
  >
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduce(MsgT *const msg, ReduceStamp stamp = ReduceStamp{}) const
  {
    return reduce<
      OpT,
      FunctorT,
      MsgT,
      &MsgT::template msgHandler<MsgT, OpT, FunctorT>
      >(msg, stamp);
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduce(
    MsgT *const msg, ReduceStamp stamp = ReduceStamp{},
    NodeType const& node = uninitialized_destination
  ) const;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduceExpr(
    MsgT *const msg, ReduceIdxFuncType fn, ReduceStamp stamp = ReduceStamp{},
    NodeType const& node = uninitialized_destination
  ) const;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduce(MsgT *const msg, ReduceStamp stamp, IndexT const& idx) const;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  [[deprecated("Use new interface calls (allreduce/reduce) without message")]]
  messaging::PendingSend reduceExpr(
    MsgT *const msg, ReduceIdxFuncType fn, ReduceStamp stamp, IndexT const& idx
  ) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_REDUCABLE_REDUCABLE_H*/
