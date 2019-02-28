/*
//@HEADER
// ************************************************************************
//
//                        proxy_objgroup.h
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

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/objgroup/proxy/proxy_objgroup_elm.h"
#include "vt/objgroup/active_func/active_func.h"
#include "vt/messaging/message/smart_ptr.h"

namespace vt { namespace objgroup { namespace proxy {

template <typename ObjT>
struct Proxy {

  Proxy() = default;
  Proxy(Proxy const&) = default;
  Proxy(Proxy&&) = default;
  Proxy& operator=(Proxy const&) = default;

  explicit Proxy(ObjGroupProxyType in_proxy)
    : proxy_(in_proxy)
  { }

public:

  /*
   * Broadcast a msg to this object group with a handler
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcast(MsgT* msg) const;
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcast(MsgSharedPtr<MsgT> msg) const;
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
  void broadcast(Args&&... args) const;

  /*
   * Get the local pointer to this object group residing in the current node
   * context
   */
  ObjT* get() const;

public:

  /*
   * Index the object group to get an element; can use operator[] or operator()
   */
  ProxyElm<ObjT> operator[](NodeType node) const;
  ProxyElm<ObjT> operator()(NodeType node) const;

private:
  ObjGroupProxyType proxy_ = no_obj_group;
};

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_H*/
