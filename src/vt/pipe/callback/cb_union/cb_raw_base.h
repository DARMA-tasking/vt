/*
//@HEADER
// *****************************************************************************
//
//                                cb_raw_base.h
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

#if !defined INCLUDED_VT_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H
#define INCLUDED_VT_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H

#include "vt/config.h"
#include "vt/pipe/callback/cb_union/cb_raw.h"
#include "vt/pipe/signal/signal.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/utils/fntraits/fntraits.h"
#include "vt/messaging/param_msg.h"

#include <cassert>
#include <type_traits>

namespace vt { namespace pipe { namespace callback { namespace cbunion {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct RawAnonTagType        { } RawAnonTag        { };
static struct RawSendMsgTagType     { } RawSendMsgTag     { };
static struct RawBcastMsgTagType    { } RawBcastMsgTag    { };
static struct RawSendColMsgTagType  { } RawSendColMsgTag  { };
static struct RawBcastColMsgTagType { } RawBcastColMsgTag { };
static struct RawSendColDirTagType  { } RawSendColDirTag  { };
static struct RawBcastColDirTagType { } RawBcastColDirTag { };
static struct RawSendObjGrpTagType  { } RawSendObjGrpTag  { };
static struct RawBcastObjGrpTagType { } RawBcastObjGrpTag { };
#pragma GCC diagnostic pop

template <typename... Args>
struct CallbackTyped;

struct CallbackRawBaseSingle {
  using AutoHandlerType = auto_registry::AutoHandlerType;

  CallbackRawBaseSingle() = default;
  CallbackRawBaseSingle(CallbackRawBaseSingle const&) = default;
  CallbackRawBaseSingle(CallbackRawBaseSingle&&) = default;
  CallbackRawBaseSingle& operator=(CallbackRawBaseSingle const&) = default;

  // Conversion operator for moving from typed to untyped
  template <typename MsgT>
  CallbackRawBaseSingle(CallbackTyped<MsgT> in);

  // Constructors for different types of callbacks
  CallbackRawBaseSingle(
    RawSendMsgTagType, PipeType const& in_pipe, HandlerType const in_handler,
    NodeType const& in_node
  );
  CallbackRawBaseSingle(
    RawBcastMsgTagType, PipeType const& in_pipe, HandlerType const in_handler
  );
  CallbackRawBaseSingle(RawAnonTagType, PipeType const& in_pipe);
  CallbackRawBaseSingle(RawSendColMsgTagType, PipeType const& in_pipe);
  CallbackRawBaseSingle(RawBcastColMsgTagType, PipeType const& in_pipe);
  CallbackRawBaseSingle(
    RawBcastColDirTagType, PipeType const& in_pipe,
    HandlerType const in_handler, AutoHandlerType const in_vrt,
    VirtualProxyType const& in_proxy
  );
  CallbackRawBaseSingle(
    RawSendColDirTagType, PipeType const& in_pipe,
    HandlerType const in_handler, AutoHandlerType const in_vrt_handler,
    void* index_bits
  );
  CallbackRawBaseSingle(
    RawBcastObjGrpTagType, PipeType in_pipe, HandlerType in_handler,
    ObjGroupProxyType in_proxy
  );
  CallbackRawBaseSingle(
    RawSendObjGrpTagType, PipeType in_pipe, HandlerType in_handler,
    ObjGroupProxyType in_proxy, NodeType in_node
  );

  template <typename MsgT>
  bool operator==(CallbackTyped<MsgT> const& other)   const;
  bool operator==(CallbackRawBaseSingle const& other) const {
    return equal(other);
  }

  template <typename CallbackT>
  bool equal(CallbackT const& other) const {
    return other.pipe_ == pipe_ && other.cb_ == cb_;
  }

  bool null()  const { return cb_.null();  }
  bool valid() const { return cb_.valid(); }

  template <typename MsgT>
  void send(MsgT* msg) {
    sendMsg<MsgT>(msg);
  }

  template <typename MsgT>
  void send(messaging::MsgPtrThief<MsgT> msg) {
    return sendMsg(msg);
  }

  template <typename MsgT>
  void sendMsg(MsgT* msg);

  template <typename MsgT>
  void sendMsg(messaging::MsgPtrThief<MsgT> msg);

  void sendVoid();

  template <typename SerializerT>
  void serialize(SerializerT& s);

  PipeType getPipe() const { return pipe_; }

  template <typename... Args>
  friend struct CallbackTyped;

protected:
  PipeType pipe_ = no_pipe;
  GeneralCallback cb_;
};

template <typename... Args>
struct CallbackTyped : CallbackRawBaseSingle {
  using TupleType = std::tuple<Args...>;

  CallbackTyped() = default;
  CallbackTyped(CallbackTyped const&) = default;
  CallbackTyped(CallbackTyped&&) = default;
  CallbackTyped& operator=(CallbackTyped const&) = default;

  // Forwarding constructors for different types of callbacks
  CallbackTyped(
    RawSendMsgTagType, PipeType const& in_pipe, HandlerType const in_handler,
    NodeType const& in_node
  ) : CallbackRawBaseSingle(RawSendMsgTag,in_pipe,in_handler,in_node)
  { }
  CallbackTyped(
    RawBcastMsgTagType, PipeType const& in_pipe, HandlerType const in_handler
  )  : CallbackRawBaseSingle(RawBcastMsgTag,in_pipe,in_handler)
  { }
  CallbackTyped(RawAnonTagType, PipeType const& in_pipe)
    : CallbackRawBaseSingle(RawAnonTag,in_pipe)
  { }
  CallbackTyped(RawSendColMsgTagType, PipeType const& in_pipe)
    : CallbackRawBaseSingle(RawSendColMsgTag,in_pipe)
  { }
  CallbackTyped(RawBcastColMsgTagType, PipeType const& in_pipe)
    : CallbackRawBaseSingle(RawBcastColMsgTag,in_pipe)
  { }
  CallbackTyped(
    RawBcastColDirTagType, PipeType const& in_pipe,
    HandlerType const in_handler, AutoHandlerType const in_vrt,
    VirtualProxyType const& in_proxy
  ) : CallbackRawBaseSingle(
        RawBcastColDirTag, in_pipe, in_handler, in_vrt, in_proxy
      )
  { }
  CallbackTyped(
    RawSendColDirTagType, PipeType const& in_pipe,
    HandlerType const in_handler, AutoHandlerType const in_vrt_handler,
    void* index_bits
  ) : CallbackRawBaseSingle(
        RawSendColDirTag,in_pipe,in_handler,in_vrt_handler,index_bits
      )
  { }
  CallbackTyped(
    RawBcastObjGrpTagType, PipeType in_pipe, HandlerType in_handler,
    ObjGroupProxyType in_proxy
  )  : CallbackRawBaseSingle(
        RawBcastObjGrpTag,in_pipe,in_handler,in_proxy
      )
  { }
  CallbackTyped(
    RawSendObjGrpTagType, PipeType in_pipe, HandlerType in_handler,
    ObjGroupProxyType in_proxy, NodeType in_node
  )  : CallbackRawBaseSingle(
        RawSendObjGrpTag,in_pipe,in_handler,in_proxy,in_node
      )
  { }

  bool operator==(CallbackTyped<Args...> const& other)   const {
    return equal(other);
  }
  bool operator==(CallbackRawBaseSingle const& other) const {
    return equal(other);
  }

  template <typename CallbackT>
  bool equal(CallbackT const& other) const {
    return other.pipe_ == pipe_ && other.cb_ == cb_;
  }

  template <typename... Params>
  void sendTuple(std::tuple<Params...> tup) {
    using Trait = CBTraits<Args...>;
    using MsgT = messaging::ParamMsg<typename Trait::TupleType>;
    auto msg = vt::makeMessage<MsgT>(std::move(tup));
    CallbackRawBaseSingle::sendMsg<MsgT>(msg);
  }

  template <typename... Params>
  void send(Params&&... params) {
    using Trait = CBTraits<Args...>;
    if constexpr (std::is_same_v<typename Trait::MsgT, NoMsg>) {
      using MsgT = messaging::ParamMsg<typename Trait::TupleType>;
      auto msg = vt::makeMessage<MsgT>(std::forward<Params>(params)...);
      CallbackRawBaseSingle::sendMsg<MsgT>(msg);
    } else {
      using MsgT = typename Trait::MsgT;
      auto msg = makeMessage<MsgT>(std::forward<Params>(params)...);
      sendMsg(msg.get());
    }
  }

  void send(typename CBTraits<Args...>::MsgT* msg) {
    using MsgT = typename CBTraits<Args...>::MsgT;
    if constexpr (not std::is_same_v<MsgT, NoMsg>) {
      CallbackRawBaseSingle::sendMsg<MsgT>(msg);
    }
  }

  template <typename MsgT>
  void send(messaging::MsgPtrThief<MsgT> msg) {
    CallbackRawBaseSingle::sendMsg<MsgT>(msg);
  }

  void sendMsg(messaging::MsgPtrThief<typename CBTraits<Args...>::MsgT> msg) {
    using MsgT = typename CBTraits<Args...>::MsgT;
    if constexpr (not std::is_same_v<MsgT, NoMsg>) {
      CallbackRawBaseSingle::sendMsg<MsgT>(msg);
    }
  }

  void sendMsg(typename CBTraits<Args...>::MsgT* msg) {
    using MsgT = typename CBTraits<Args...>::MsgT;
    if constexpr (not std::is_same_v<MsgT, NoMsg>) {
      CallbackRawBaseSingle::sendMsg<MsgT>(msg);
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackRawBaseSingle::serialize(s);
  }
};

}}}} /* end namespace vt::pipe::callback::cbunion */

namespace vt {

template <typename... Args>
using Callback = pipe::callback::cbunion::CallbackTyped<Args...>;

using CallbackU = pipe::callback::cbunion::CallbackRawBaseSingle;

} /* end namespace vt */

#include "vt/pipe/callback/cb_union/cb_raw_base.impl.h"

#endif /*INCLUDED_VT_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H*/
