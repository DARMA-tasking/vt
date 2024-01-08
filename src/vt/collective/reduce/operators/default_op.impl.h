/*
//@HEADER
// *****************************************************************************
//
//                              default_op.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_OP_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_OP_IMPL_H

#include "vt/config.h"
#include "vt/collective/reduce/operators/default_op.h"

namespace vt { namespace collective { namespace reduce { namespace operators {

struct NoCombine {};

template <typename>
struct IsTuple : std::false_type {};
template <typename... Args>
struct IsTuple<std::tuple<Args...>> : std::true_type {};

template <typename T>
template <typename MsgT, typename Op, typename ActOp>
/*static*/ void ReduceCombine<T>::msgHandler(MsgT* msg) {
  if (msg->isRoot()) {
    vt_debug_print(
      terse, reduce,
      "ROOT: reduce root: ptr={}\n", print_ptr(msg)
    );
    if (msg->hasValidCallback()) {
      envelopeUnlockForForwarding(msg->env);
      if (msg->isParamCallback()) {
        if constexpr (IsTuple<typename MsgT::DataT>::value) {
          msg->getParamCallback().sendTuple(std::move(msg->getVal()));
        }
      } else {
        // We need to force the type to the more specific one here
        auto cb = msg->getMsgCallback();
        auto typed_cb = reinterpret_cast<Callback<MsgT>*>(&cb);
        typed_cb->sendMsg(msg);
      }
    } else if (msg->root_handler_ != uninitialized_handler) {
      auto_registry::getAutoHandler(msg->root_handler_)->dispatch(msg, nullptr);
    } else {
      if constexpr (not std::is_same_v<ActOp, NoCombine>) {
        ActOp()(msg);
      }
    }
  } else {
    MsgT* fst_msg = msg;
    MsgT* cur_msg = msg->template getNext<MsgT>();
    vt_debug_print(
      verbose, reduce,
      "leaf: fst ptr={}\n", print_ptr(fst_msg)
    );
    while (cur_msg != nullptr) {
      ReduceCombine<>::combine<MsgT,Op,ActOp>(fst_msg, cur_msg);
      cur_msg = cur_msg->template getNext<MsgT>();
    }
  }
}

}}}} /* end namespace vt::collective::reduce::operators */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_OP_IMPL_H*/
