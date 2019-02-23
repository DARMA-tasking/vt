/*
//@HEADER
// ************************************************************************
//
//                          pipe_manager.h
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

#if !defined INCLUDED_PIPE_PIPE_MANAGER_H
#define INCLUDED_PIPE_PIPE_MANAGER_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/pipe_manager.fwd.h"
#include "vt/pipe/pipe_manager_tl.h"
#include "vt/pipe/pipe_manager_typed.h"
#include "vt/pipe/msg/callback.h"
#include "vt/pipe/signal/signal_holder.h"
#include "vt/pipe/callback/anon/callback_anon.fwd.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/activefn/activefn.h"

#include <unordered_map>
#include <functional>

namespace vt { namespace pipe {


template <typename T>
struct FunctorTraits {
  template <typename U>
  using FunctorNoMsgArchType = decltype(std::declval<U>().operator()());
  using FunctorNoMsgType = detection::is_detected<FunctorNoMsgArchType,T>;
  static constexpr auto const has_no_msg_type = FunctorNoMsgType::value;
};

struct PipeManager : PipeManagerTL, PipeManagerTyped {

  template <typename FunctorT>
  using GetMsgType = typename util::FunctorExtractor<FunctorT>::MessageType;
  using Void = V;

  PipeManager();

  template <typename MsgT, typename ContextT>
  Callback<MsgT> makeFunc(ContextT* ctx, FuncMsgCtxType<MsgT, ContextT> fn);
  template <typename ContextT>
  Callback<Void> makeFunc(ContextT* ctx, FuncCtxType<ContextT> fn);
  template <typename MsgT>
  Callback<MsgT> makeFunc(FuncMsgType<MsgT> fn);
  Callback<Void> makeFunc(FuncVoidType fn);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  Callback<MsgT> makeSend(NodeType const& node);
  template <typename FunctorT, typename MsgT = GetMsgType<FunctorT>>
  Callback<MsgT> makeSend(NodeType const& node);
  template <
    typename FunctorT,
    typename = std::enable_if_t<FunctorTraits<FunctorT>::has_no_msg_type>
  >
  Callback<Void> makeSend(NodeType const& node);
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  Callback<MsgT> makeSend(typename ColT::ProxyType proxy);
  template <typename ColT, typename MsgT, ColMemType<ColT,MsgT> f>
  Callback<MsgT> makeSend(typename ColT::ProxyType proxy);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  Callback<MsgT> makeBcast();
  template <typename FunctorT, typename MsgT = GetMsgType<FunctorT>>
  Callback<MsgT> makeBcast();
  template <
    typename FunctorT,
    typename = std::enable_if_t<FunctorTraits<FunctorT>::has_no_msg_type>
  >
  Callback<Void> makeBcast();
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  Callback<MsgT> makeBcast(ColProxyType<ColT> proxy);
  template <typename ColT, typename MsgT, ColMemType<ColT,MsgT> f>
  Callback<MsgT> makeBcast(ColProxyType<ColT> proxy);


public:
  /*
   *  Trigger and send back on a pipe that is not locally triggerable and thus
   *  requires communication if it is "sent" off-node.
   */
  template <typename MsgT>
  void triggerSendBack(PipeType const& pipe, MsgT* data);

private:
  // The group ID used to indicate that the message is being used as a pipe
  GroupType group_id_ = no_group;
};

}} /* end namespace vt::pipe */

#include "vt/pipe/pipe_manager_base.impl.h"
#include "vt/pipe/pipe_manager_tl.impl.h"
#include "vt/pipe/pipe_manager_typed.impl.h"
#include "vt/pipe/pipe_manager.impl.h"

#endif /*INCLUDED_PIPE_PIPE_MANAGER_H*/
