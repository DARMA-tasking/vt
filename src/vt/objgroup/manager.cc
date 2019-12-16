/*
//@HEADER
// *****************************************************************************
//
//                                  manager.cc
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

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/manager.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/objgroup/type_registry/registry.h"
#include "vt/context/context.h"
#include "vt/messaging/message/smart_ptr.h"

namespace vt { namespace objgroup {

ObjGroupProxyType ObjGroupManager::getProxy(ObjGroupProxyType proxy) {
  return proxy;
}

void ObjGroupManager::dispatch(MsgVirtualPtrAny msg, HandlerType han) {
  // Extract the control-bit sequence from the handler
  auto const ctrl = HandlerManager::getHandlerControl(han);
  auto const type_idx = auto_registry::getAutoHandlerObjTypeIdx(han);
  debug_print(
    objgroup, node,
    "dispatch: type_idx={:x}, ctrl={:x}, han={:x}\n", type_idx, ctrl, han
  );
  auto const node = 0;
  auto const proxy = proxy::ObjGroupProxy::create(ctrl,type_idx,node,true);
  auto dispatch_iter = dispatch_.find(proxy);
  debug_print(
    objgroup, node,
    "dispatch: try type_idx={:x}, ctrl={:x}, han={:x}, has dispatch={}\n",
    type_idx, ctrl, han, dispatch_iter != dispatch_.end()
  );
  if (dispatch_iter == dispatch_.end()) {
    auto const epoch = envelopeGetEpoch(msg->env);
    if (epoch != no_epoch and epoch != term::any_epoch_sentinel) {
      theTerm()->produce(epoch);
    }
    pending_[proxy].push_back(msg);
  } else {
    dispatch_iter->second->run(han,msg.get());
  }
}

ObjGroupProxyType ObjGroupManager::makeCollectiveImpl(
  HolderBasePtrType base, ObjTypeIdxType idx, void* obj_ptr
) {
  auto iter = cur_obj_id_.find(idx);
  if (iter == cur_obj_id_.end()) {
    cur_obj_id_[idx] = fst_obj_group_id;
    iter = cur_obj_id_.find(idx);
  }
  vtAssert(iter != cur_obj_id_.end(), "Must have valid type idx lookup");
  auto const id = iter->second++;
  auto const node = theContext()->getNode();
  auto const is_collective = true;
  auto const proxy = proxy::ObjGroupProxy::create(id, idx, node, is_collective);

  obj_to_proxy_[obj_ptr] = proxy;

  auto obj_iter = objs_.find(proxy);
  vtAssert(obj_iter == objs_.end(), "Proxy must not exist in obj group map");
  objs_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(std::move(base))
  );
  return proxy;
}

bool ObjGroupManager::progress() {
  return false;
}

void dispatchObjGroup(MsgVirtualPtrAny msg, HandlerType han) {
  debug_print(
    objgroup, node,
    "dispatchObjGroup: han={:x}\n", han
  );
  return theObjGroup()->dispatch(msg,han);
}

void scheduleMsg(MsgVirtualPtrAny msg, HandlerType han, EpochType epoch) {
  // Produce here, consume when the dispatcher actually runs---it might be
  // delayed
  theTerm()->produce(epoch);
  // Schedule the work of dispatching the message handler for later
  theSched()->enqueue([msg,han,epoch]{
    auto const node = theContext()->getNode();
    runnable::Runnable<ShortMessage>::runObj(han,msg.get(),node);
    theTerm()->consume(epoch);
  });
}

}} /* end namespace vt::objgroup */
