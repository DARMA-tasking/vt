/*
//@HEADER
// ************************************************************************
//
//                           manager.h
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_H
#define INCLUDED_VT_OBJGROUP_MANAGER_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/holder/holder.h"

#include <memory>
#include <functional>
#include <unordered_map>

namespace vt { namespace objgroup {

struct ObjGroupManager {
  template <typename ObjT>
  using ProxyType     = proxy::Proxy<ObjT>;
  template <typename ObjT>
  using ProxyElmType  = proxy::ProxyElm<ObjT>;
  template <typename ObjT>
  using MakeFnType    = std::function<std::unique_ptr<ObjT>()>;
  template <typename ObjT>
  using HolderPtr     = std::unique_ptr<holder::Holder<ObjT>>;
  // using HolderBasePtr = std::unique_ptr<holder::HolderBase>;

  /*
   * Creation of a new object group across the distributed system. For now,
   * these use the default group which includes all the nodes in the
   * communicator
   */
  template <typename ObjT>
  ProxyType<ObjT> makeObjGroup();

  template <typename ObjT>
  ProxyType<ObjT> makeObjGroupCollective(TagType tag = no_tag);
  template <typename ObjT>
  ProxyType<ObjT> makeObjGroupCollective(std::unique_ptr<ObjT> obj);
  template <typename ObjT>
  ProxyType<ObjT> makeObjGroupCollective(MakeFnType<ObjT> fn);

  /*
   * Send/broadcast messages to obj group handlers
   */

  template <typename ObjT, typename MsgT>
  void send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  template <typename ObjT, typename MsgT>
  void broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  /*
   * Get access to the local instance of a particular object group
   */

  template <typename ObjT>
  void get(ProxyType<ObjT> proxy);

  template <typename ObjT>
  void get(ProxyElmType<ObjT> proxy);

private:
  template <typename ObjT>
  static std::unordered_map<ObjGroupProxyType,HolderPtr<ObjT>> objs_;

  static ObjGroupIDType cur_obj_id_;
};

template <typename ObjT>
/*static*/
std::unordered_map<ObjGroupProxyType,ObjGroupManager::HolderPtr<ObjT>>
ObjGroupManager::objs_ = {};

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_H*/
