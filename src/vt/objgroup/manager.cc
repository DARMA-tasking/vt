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
#include "vt/context/context.h"

namespace vt { namespace objgroup {

void ObjGroupManager::dispatch(MsgSharedPtr<ShortMessage> msg, HandlerType han) {
  auto iter = han_to_proxy_.find(han);
  vtAssertExpr(iter != han_to_proxy_.end());
  auto const proxy = iter->second;
  auto dispatch_iter = dispatch_.find(proxy);
  vtAssertExpr(dispatch_iter != dispatch_.end());
  dispatch_iter->second->run(han,msg);
}

void ObjGroupManager::regHan(ObjGroupProxyType proxy, HandlerType han) {
  auto iter = han_to_proxy_.find(han);
  vtAssertExpr(iter == han_to_proxy_.end());
  han_to_proxy_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(han),
    std::forward_as_tuple(proxy)
  );
}

ObjGroupProxyType ObjGroupManager::makeCollectiveImpl(HolderBasePtrType base) {
  auto const node = theContext()->getNode();
  auto const id = cur_obj_id_++;
  auto const is_collective = true;
  auto const coll_proxy = proxy::ObjGroupProxy::create(id, node, is_collective);
  auto iter = objs_.find(coll_proxy);
  vtAssert(iter == objs_.end(), "Proxy must not exist in obj group map");
  objs_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(coll_proxy),
    std::forward_as_tuple(std::move(base))
  );
  return coll_proxy;
}

}} /* end namespace vt::objgroup */
