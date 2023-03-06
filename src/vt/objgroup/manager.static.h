/*
//@HEADER
// *****************************************************************************
//
//                               manager.static.h
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_STATIC_H
#define INCLUDED_VT_OBJGROUP_MANAGER_STATIC_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/objgroup/holder/holder_base.h"
#include "vt/messaging/active.h"
#include "vt/runnable/make_runnable.h"

namespace vt { namespace objgroup {

template <typename MsgT>
messaging::PendingSend send(MsgSharedPtr<MsgT> msg, HandlerType han, NodeType dest_node) {
  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  vtAssert(dest_node < num_nodes, "Invalid node (must be < num_nodes)");
  if (dest_node != this_node) {
    return theMsg()->sendMsg<MsgT>(dest_node, han,msg, no_tag);
  } else {
    theMsg()->setupEpochMsg(msg);
    envelopeSetHandler(msg->env, han);
    return messaging::PendingSend{msg, [](MsgSharedPtr<BaseMsgType>& inner_msg){
      dispatchObjGroup(
        inner_msg.template to<ShortMessage>(),
        envelopeGetHandler(inner_msg->env),
        theContext()->getNode(),
        nullptr
      );
    }};
  }
}

template <typename ObjT, typename MsgT, auto f>
decltype(auto) invoke(
  messaging::MsgSharedPtr<MsgT> msg, HandlerType han, NodeType dest_node
) {
  auto const this_node = theContext()->getNode();

  vtAssert(
    dest_node == this_node,
    fmt::format(
      "Attempting to invoke handler on node:{} instead of node:{}!", this_node,
      dest_node
    )
  );

  // this is a local invocation.. no thread required
  auto holder = detail::getHolderBase(han);
  auto const& elm_id = holder->getElmID();
  auto elm = holder->getPtr();
  auto lb_data = &holder->getLBData();
  return runnable::makeRunnableVoid(false, han, this_node)
    .withObjGroup(elm)
    .withLBData(lb_data, elm_id)
    .runLambda(f, static_cast<ObjT*>(elm), msg.get());
}

template <typename MsgT>
messaging::PendingSend broadcast(MsgSharedPtr<MsgT> msg, HandlerType han) {
  return theMsg()->broadcastMsg<MsgT>(han, msg);
}

namespace detail {

template <typename MsgT, typename ObjT>
void dispatchImpl(
  MsgSharedPtr<MsgT> const& msg, HandlerType han, NodeType from_node,
  ActionType cont, ObjT* obj
) {
  auto holder = detail::getHolderBase(han);
  auto const& elm_id = holder->getElmID();
  auto lb_data = &holder->getLBData();
  runnable::makeRunnable(msg, true, han, from_node)
    .withContinuation(cont)
    .withObjGroup(obj)
    .withLBData(lb_data, elm_id)
    .withTDEpochFromMsg()
    .enqueue();
}

template <typename MsgT>
void dispatch(
  MsgSharedPtr<MsgT> msg, HandlerType han, NodeType from_node,
  ActionType cont
) {
  // Extract the control-bit sequence from the handler
  auto const ctrl = HandlerManager::getHandlerControl(han);
  vt_debug_print(
    verbose, objgroup,
    "dispatch: ctrl={:x}, han={:x}\n", ctrl, han
  );
  auto const node = 0;
  auto const proxy = proxy::ObjGroupProxy::create(ctrl, node, true);
  auto& objs = getObjs();
  auto obj_iter = objs.find(proxy);
  vt_debug_print(
    normal, objgroup,
    "dispatch: try ctrl={:x}, han={:x}, has dispatch={}\n",
    ctrl, han, obj_iter != objs.end()
  );
  if (obj_iter == objs.end()) {
    auto const epoch = envelopeGetEpoch(msg->env);
    if (epoch != no_epoch) {
      theTerm()->produce(epoch);
    }
    auto& pending = getPending();
    pending[proxy].emplace_back([=]{
      auto& objs2 = getObjs();
      auto obj_iter2 = objs2.find(proxy);
      vtAssert(obj_iter2 != objs2.end(), "Obj must exist");
      detail::dispatchImpl(msg, han, from_node, cont, obj_iter2->second->getPtr());
      if (epoch != no_epoch) {
        theTerm()->consume(epoch);
      }
    });
  } else {
    detail::dispatchImpl(msg, han, from_node, cont, obj_iter->second->getPtr());
  }
}

} /* end namespace detail */

template <typename MsgT>
void dispatchObjGroup(
  MsgSharedPtr<MsgT> msg, HandlerType han, NodeType from_node,
  ActionType cont
) {
  vt_debug_print(
    verbose, objgroup,
    "dispatchObjGroup: han={:x}\n", han
  );
  return detail::dispatch(msg, han, from_node, cont);
}

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_STATIC_H*/
