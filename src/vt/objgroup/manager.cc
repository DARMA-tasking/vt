/*
//@HEADER
// ************************************************************************
//
//                            manager.cc
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

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/manager.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/objgroup/type_registry/registry.h"
#include "vt/context/context.h"

namespace vt { namespace objgroup {

void scheduleMsg(MsgSharedPtr<ShortMessage> msg, HandlerType han) {
  // Schedule the work of dispatching the message handler for later
  theObjGroup()->work_units_.push_back(
    [msg,han]{ theObjGroup()->dispatch(msg,han); }
  );
}

void ObjGroupManager::dispatch(MsgSharedPtr<ShortMessage> msg, HandlerType han) {
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
  if (dispatch_iter == dispatch_.end()) {
    pending_[proxy].push_back(msg);
  } else {
    dispatch_iter->second->run(han,msg);
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

  vtAssertExpr(obj_to_proxy_.find(obj_ptr) == obj_to_proxy_.end());
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

bool ObjGroupManager::scheduler() {
  if (work_units_.size() == 0) {
    return false;
  } else {
    auto unit = work_units_.back();
    work_units_.pop_back();
    unit();
    return true;
  }
}

bool scheduler() {
  return theObjGroup()->scheduler();
}

void dispatchObjGroup(MsgSharedPtr<ShortMessage> msg, HandlerType han) {
  debug_print(
    objgroup, node,
    "dispatchObjGroup: han={:x}\n", han
  );
  return theObjGroup()->dispatch(msg,han);
}

}} /* end namespace vt::objgroup */
