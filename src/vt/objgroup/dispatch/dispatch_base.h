/*
//@HEADER
// ************************************************************************
//
//                          dispatch_base.h
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
  virtual void run(HandlerType han, MsgSharedPtr<ShortMessage> msg) = 0;

  ObjGroupProxyType proxy() const { return proxy_; }

private:
  ObjGroupProxyType proxy_ = no_obj_group;
};

}}} /* end namespace vt::objgroup::dispatch */

#endif /*INCLUDED_VT_OBJGROUP_DISPATCH_DISPATCH_BASE_H*/
