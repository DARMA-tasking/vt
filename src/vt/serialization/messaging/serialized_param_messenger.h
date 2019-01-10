/*
//@HEADER
// ************************************************************************
//
//                          serialized_param_messenger.h
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

#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_PARAM_MESSENGER_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_PARAM_MESSENGER_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/active.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/serialization/serialization.h"
#include "vt/serialization/messaging/serialized_data_msg.h"
#include "vt/parameterization/param_meta.h"

#include <tuple>
#include <utility>

namespace vt { namespace serialization {

using namespace ::vt::param;

#define SERIAL_MSG_HAN(value) PARAM_FUNCTION(value)

template<typename T, T value>
using SerializedNonType = ::vt::param::NonType<T, value>;

struct SerializedMessengerParam {
  template <typename T>
  using MsgType = SerializedDataMsg<T>;

  template <typename Tuple>
  static void serializedMsgHandler(MsgType<Tuple>* msg) {
    auto fn = auto_registry::getAutoHandler(msg->handler);
    auto const& recv_tag = msg->data_recv_tag;
    theMsg()->recvDataMsg(
      recv_tag, msg->from_node, [=](RDMA_GetType ptr, ActionType){
        auto raw_ptr = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
        auto ptr_size = std::get<1>(ptr);
        auto& t1 = deserialize<Tuple>(raw_ptr, ptr_size, nullptr);
        invokeCallableTuple(std::forward<Tuple>(t1), fn, false);
      }
    );
  }

  template <typename T, T value, typename... Args>
  static void sendSerdesParamMsg(
    NodeType const& dest, std::tuple<Args...> tup,
    SerializedNonType<T, value> __attribute__((unused)) non = SerializedNonType<T,value>()
  ) {
    auto const& typed_handler = auto_registry::makeAutoHandler<T,value>();

    using TupleType = std::tuple<Args...>;

    auto meta_typed_data_msg = new SerializedDataMsg<TupleType>();

    SerialByteType* ptr = nullptr;
    SizeType ptr_size = 0;

    auto serialized = serialize(tup, [&](SizeType size) -> SerialByteType*{
      ptr_size = size;
      ptr = static_cast<SerialByteType*>(malloc(size));
      return ptr;
    });

    auto send_serialized = [&](Active::SendFnType send){
      auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag, no_action);
      meta_typed_data_msg->data_recv_tag = std::get<1>(ret);
    };

    meta_typed_data_msg->handler = typed_handler;
    meta_typed_data_msg->from_node = theContext()->getNode();

    auto deleter = [=]{ delete meta_typed_data_msg; };

    theMsg()->sendMsg<MsgType<TupleType>, serializedMsgHandler>(
      dest, meta_typed_data_msg, send_serialized, deleter
    );
  }

};


}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_PARAM_MESSENGER_H*/
