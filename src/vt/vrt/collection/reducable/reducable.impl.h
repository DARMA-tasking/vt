/*
//@HEADER
// *****************************************************************************
//
//                               reducable.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/reducable/reducable.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Reducable<ColT,IndexT,BaseProxyT>::Reducable(VirtualProxyType const in_proxy)
  : BaseProxyT(in_proxy)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename OpT, typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend Reducable<ColT,IndexT,BaseProxyT>::reduce(
  MsgT *const msg, Callback<MsgT> cb, ReduceStamp stamp
) const {
  auto const proxy = this->getProxy();
  msg->setCallback(cb);
  vt_debug_print(
    normal, reduce,
    "Reducable: valid={} {}, ptr={}\n", cb.valid(), msg->getCallback().valid(),
    print_ptr(msg)
  );
  auto const root_node = 0;
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,stamp,root_node);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename OpT, typename FunctorT, typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend Reducable<ColT,IndexT,BaseProxyT>::reduce(
  MsgT *const msg, ReduceStamp stamp
) const {
  auto const proxy = this->getProxy();
  auto const root_node = 0;
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,stamp,root_node);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend Reducable<ColT,IndexT,BaseProxyT>::reduce(
  MsgT *const msg, ReduceStamp stamp, NodeType const& node
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,stamp,node);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend Reducable<ColT,IndexT,BaseProxyT>::reduceExpr(
  MsgT *const msg, ReduceIdxFuncType fn, ReduceStamp stamp, NodeType const& node
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsgExpr<ColT,MsgT,f>(proxy,msg,fn,stamp,node);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend Reducable<ColT,IndexT,BaseProxyT>::reduce(
  MsgT *const msg, ReduceStamp stamp, IndexT const& idx
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,stamp,idx);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend Reducable<ColT,IndexT,BaseProxyT>::reduceExpr(
  MsgT *const msg, ReduceIdxFuncType fn, ReduceStamp stamp, IndexT const& idx
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsgExpr<ColT,MsgT,f>(proxy,msg,fn,stamp,idx);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H*/
