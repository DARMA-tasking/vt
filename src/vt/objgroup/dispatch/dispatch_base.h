/*
//@HEADER
// *****************************************************************************
//
//                               dispatch_base.h
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

#if !defined INCLUDED_VT_OBJGROUP_DISPATCH_DISPATCH_BASE_H
#define INCLUDED_VT_OBJGROUP_DISPATCH_DISPATCH_BASE_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/messaging/message/smart_ptr.h"

namespace vt { namespace objgroup { namespace dispatch {

/*
 * DispatchBase implements type erasure to dispatch to a obj group without
 * encoding the message directly in the message (as an alternative to using a
 * std::function)
 */

struct DispatchBase {
  explicit DispatchBase(ObjGroupProxyType in_proxy)
    : proxy_(in_proxy)
  { }

  virtual ~DispatchBase() = default;

  /*
   * Dispatch to the handler; the base is closed around the proper object
   * pointer that is type-erased here
   */
  virtual void run(
    HandlerType han, BaseMessage* msg, NodeType from_node, ActionType cont
  ) = 0;
  virtual void* objPtr() const = 0;

  ObjGroupProxyType proxy() const { return proxy_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | proxy_;
  }

private:
  ObjGroupProxyType proxy_ = no_obj_group;
};

}}} /* end namespace vt::objgroup::dispatch */

#endif /*INCLUDED_VT_OBJGROUP_DISPATCH_DISPATCH_BASE_H*/
