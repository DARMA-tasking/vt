/*
//@HEADER
// *****************************************************************************
//
//                              parameterization.h
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

#if !defined INCLUDED_VT_PARAMETERIZATION_PARAMETERIZATION_H
#define INCLUDED_VT_PARAMETERIZATION_PARAMETERIZATION_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/active.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/utils/static_checks/all_true.h"
#include "vt/parameterization/param_meta.h"
#include "vt/runtime/component/component_pack.h"

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace vt { namespace param {

using namespace ::vt::util;

using HandlerManagerType = vt::HandlerManager;

template <typename Tuple>
struct DataMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_if_needed_by_parent_or_type1(Tuple); // by tup

  Tuple tup;
  HandlerType sub_han = uninitialized_handler;

  DataMsg() = default;
  DataMsg(HandlerType const in_sub_han, Tuple&& a)
    : Message(), tup(std::forward<Tuple>(a)), sub_han(in_sub_han)
  { }

  template <typename... Args>
  DataMsg(HandlerType const in_sub_han, Args&&... a)
    : Message(), tup(std::forward<Args>(a)...), sub_han(in_sub_han)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | tup;
    s | sub_han;
  }
};


template <typename Tuple>
static void dataMessageHandler(DataMsg<Tuple>* msg) {
  vt_debug_print(
    normal, param,
    "dataMessageHandler: id={}\n", msg->sub_han
  );

#if vt_check_enabled(trace_enabled)
  trace::TraceProcessingTag processing_tag;
  {
    trace::TraceEntryIDType ep = auto_registry::handlerTraceID(msg->sub_han);
    trace::TraceEventIDType event = envelopeGetTraceEvent(msg->env);

    size_t msg_size = sizeof(*msg);
    NodeType const& from_node = theContext()->getFromNodeCurrentTask();

    processing_tag =
      theTrace()->beginProcessing(ep, msg_size, event, from_node);
  }
#endif

  if (HandlerManagerType::isHandlerFunctor(msg->sub_han)) {
    auto const& fn = auto_registry::getAutoHandlerFunctor(msg->sub_han);
    invokeCallableTuple(msg->tup, fn, true);
  } else {
    // regular active function
    auto const& fn = auto_registry::getAutoHandler(msg->sub_han);
    invokeCallableTuple(msg->tup, fn, false);
  }

#if vt_check_enabled(trace_enabled)
  theTrace()->endProcessing(processing_tag);
#endif
}

/**
 * \struct Param
 *
 * \brief An experimental component for parameterizing handlers.
 *
 * Clean support for parameterization does not exist until C++17. This component
 * is an attempt at parameterizing for C++14 with non-type templates.
 *
 * \warning This is an experimental component.
 */
struct Param : runtime::component::Component<Param> {

  std::string name() override { return "Param"; }

  template <typename... Args>
  void sendDataTuple(
    NodeType const& dest, HandlerType const han, std::tuple<Args...>&& tup
  ) {
    staticCheckCopyable<Args...>();

    using TupleType = typename std::decay<decltype(tup)>::type;

    auto m = makeMessage<DataMsg<TupleType>>(
      han, std::forward<std::tuple<Args...>>(tup)
    );
    theMsg()->sendMsg<DataMsg<TupleType>, dataMessageHandler>(dest, m);
  }

  template <typename... Args>
  void staticCheckCopyable() {
    using cond = all_true<std::is_trivially_copyable<Args>::value...>;

    static_assert(
      std::is_same<typename cond::type,std::true_type>::value == true,
      "All types passed for parameterization must be trivially copyable"
    );
  }

  template <typename DataMsg>
  void sendDataMsg(
    NodeType const& dest, HandlerType const __attribute__((unused)) han,
    MsgSharedPtr<DataMsg> m
  ) {
    auto pmsg = promoteMsg(m.get());
    theMsg()->sendMsg<DataMsg, dataMessageHandler>(dest, pmsg);
  }

  template <typename T, T value, typename Tuple>
  void sendData(
    NodeType const& dest, Tuple tup,
    NonType<T, value> __attribute__((unused)) non = NonType<T,value>()
  ) {
    auto const& han = auto_registry::makeAutoHandlerParam<T,value>();
    sendDataTuple(dest, han, std::forward<Tuple>(tup));
  }

  template <typename T, T value, typename... Args>
  void sendData(
    NodeType const& dest, MsgSharedPtr<DataMsg<std::tuple<Args...>>> msg,
    NonType<T, value> __attribute__((unused)) non = NonType<T,value>()
  ) {
    auto const& han = auto_registry::makeAutoHandlerParam<T,value>();
    msg->sub_han = han;
    sendDataMsg(dest, han, msg);
  }

  template <typename T, T value, typename... Args>
  void sendData(
    NodeType const& dest, NonType<T, value> __attribute__((unused)) non,
    Args&&... a
  ) {
    auto const& han = auto_registry::makeAutoHandlerParam<T,value>();

    staticCheckCopyable<Args...>();

    using TupleType = std::tuple<Args...>;

    auto m = makeMessage<DataMsg<TupleType>>(han, std::forward<Args>(a)...);
    sendDataMsg(dest, han, m);
  }

  template <typename T, T value, typename... Args>
  void sendData(NodeType const& dest, Args&&... a) {
    sendData(dest, NonType<T,value>(), std::forward<Args>(a)...);
  }

  /*
   * Functor variants
   */

  template <typename FunctorT, typename... Args>
  void sendDataHelperFunctor(
    NodeType const& dest, std::tuple<Args...>&& tup
  ) {
    auto const& han = auto_registry::makeAutoHandlerFunctor<
      FunctorT, false, Args...
    >();
    sendDataTuple(dest, han, std::forward<std::tuple<Args...>>(tup));
  }

  template <typename FunctorT, typename Tuple>
  void sendData(NodeType const& dest, Tuple tup) {
    sendDataHelperFunctor<FunctorT>(dest, std::forward<Tuple>(tup));
  }

  template <typename FunctorT, typename... Args>
  void sendData(
    NodeType const& dest, MsgSharedPtr<DataMsg<std::tuple<Args...>>> msg
  ) {
    staticCheckCopyable<Args...>();

    auto const& han = auto_registry::makeAutoHandlerFunctor<
      FunctorT, false, Args...
    >();
    msg->sub_han = han;

    sendDataMsg(dest, han, msg);
  }

  template <typename FunctorT, typename... Args>
  void sendData(NodeType const& dest, Args&&... a) {
    staticCheckCopyable<Args...>();

    auto const& han = auto_registry::makeAutoHandlerFunctor<
      FunctorT, false, Args...
    >();

    using TupleType = std::tuple<Args...>;

    auto m = makeMessage<DataMsg<TupleType>>(han, std::forward<Args>(a)...);
    sendDataMsg(dest, han, m);
  }

  template <typename Serializer>
  void serialize(Serializer&) {}
};

}} //end namespace vt::param

namespace vt {

extern param::Param* theParam();

template <typename... Args>
MsgSharedPtr<param::DataMsg<std::tuple<Args...>>> buildData(Args&&... a) {
  return makeMessage<param::DataMsg<std::tuple<Args...>>>(
    uninitialized_handler, std::forward<Args>(a)...
  );
}

} //end namespace vt

#endif /*INCLUDED_VT_PARAMETERIZATION_PARAMETERIZATION_H*/
