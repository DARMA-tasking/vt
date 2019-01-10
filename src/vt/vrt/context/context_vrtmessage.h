/*
//@HEADER
// ************************************************************************
//
//                          context_vrtmessage.h
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

#if !defined INCLUDED_CONTEXT_VRT_MESSAGE
#define INCLUDED_CONTEXT_VRT_MESSAGE

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/serialization/traits/byte_copy_trait.h"
#include "vt/topos/location/message/msg.h"

#include <cassert>
#include <type_traits>

namespace vt { namespace vrt {

template <typename MessageT>
using RoutedMessageType = LocationRoutedMsg<VirtualProxyType, MessageT>;

struct VirtualMessage :
    RoutedMessageType<vt::Message>, serialization::ByteCopyTrait
{
  // By default, the `VirtualMessage' is byte copyable for serialization, but
  // derived classes may not be. The serialization::ByteCopyTrait specifies this
  // property

  VirtualMessage() = default;

  // The variable `vc_sub_handler_' manages the intended user handler the
  // VirtualMessage should trigger
  void setVrtHandler(HandlerType const& in_handler) {
    vt_sub_handler_ = in_handler;
  }
  HandlerType getVrtHandler() const {
    vtAssert(vt_sub_handler_ != uninitialized_handler, "Must have a valid handler");
    return vt_sub_handler_;
  }

  // The variable `to_proxy_' manages the intended target of the VirtualMessage
  VirtualProxyType getProxy() const {
    vtAssert(to_proxy_ != no_vrt_proxy, "Must have a valid proxy target");
    return to_proxy_;
  }
  void setProxy(VirtualProxyType const& in_proxy) { to_proxy_ = in_proxy; }

  // Force the message to always execute on the communication thread regardless
  // of the mapping of the virtual context to a core. Used for system messages.
  bool getExecuteCommThread() const { return execute_comm_thd_; }
  void setExecuteCommThread(bool const comm) { execute_comm_thd_ = comm; }

  // Explicitly write a parent serializer so derived user messages can contain
  // non-byte serialization
  template <typename SerializerT>
  void serializeParent(SerializerT& s) {
    RoutedMessageType<vt::Message>::serializeParent(s);
    RoutedMessageType<vt::Message>::serializeThis(s);
  }

  template <typename SerializerT>
  void serializeThis(SerializerT& s) {
    s | vt_sub_handler_;
    s | to_proxy_;
    s | execute_comm_thd_;
  }

private:
  bool execute_comm_thd_ = false;
  VirtualProxyType to_proxy_ = no_vrt_proxy;
  HandlerType vt_sub_handler_ = uninitialized_handler;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT_MESSAGE*/
