/*
//@HEADER
// *****************************************************************************
//
//                                cb_raw_base.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H
#define INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H

#include "vt/config.h"
#include "vt/pipe/callback/cb_union/cb_raw.h"
#include "vt/pipe/signal/signal.h"
#include "vt/registry/auto/auto_registry_common.h"

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

template <typename MsgT>
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

  template <typename MsgT, typename... Args>
  void send(Args... args);

  template <typename MsgT>
  void send(MsgT* msg);

  template <typename MsgT>
  void sendMsg(messaging::MsgPtrThief<MsgT> msg);

  void send();

  template <typename SerializerT>
  void serialize(SerializerT& s);

  PipeType getPipe() const { return pipe_; }

  template <typename MsgT>
  friend struct CallbackTyped;

protected:
  PipeType pipe_ = no_pipe;
  GeneralCallback cb_;
};

template <typename MsgT>
struct CallbackTyped : CallbackRawBaseSingle {
  using VoidSigType   = signal::SigVoidType;
  template <typename T, typename U=void>
  using IsVoidType    = std::enable_if_t<std::is_same<T,VoidSigType>::value,U>;
  template <typename T, typename U=void>
  using IsNotVoidType = std::enable_if_t<!std::is_same<T,VoidSigType>::value,U>;

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

  bool operator==(CallbackTyped<MsgT> const& other)   const {
    return equal(other);
  }
  bool operator==(CallbackRawBaseSingle const& other) const {
    return equal(other);
  }

  template <typename CallbackT>
  bool equal(CallbackT const& other) const {
    return other.pipe_ == pipe_ && other.cb_ == cb_;
  }

  // Conversion operators to typed from untyped
  CallbackTyped(CallbackRawBaseSingle const& other) {
    pipe_ = other.pipe_;
    cb_   = other.cb_;
  }
  CallbackTyped(CallbackRawBaseSingle&& other) {
    pipe_ = std::move(other.pipe_);
    cb_   = std::move(other.cb_);
  }

  template <typename MsgU=MsgT, typename... Args>
  void send(Args... args) {
    static_assert(std::is_same<MsgT, MsgU>::value, "Required exact type match");
    sendMsg(makeMessage<MsgU>(std::forward<Args>(args)...));
  }

  template <typename MsgU>
  IsNotVoidType<MsgU> send(MsgU* m) {
    static_assert(std::is_same<MsgT,MsgU>::value, "Required exact type match");
    CallbackRawBaseSingle::send<MsgU>(m);
  }

  void sendMsg(messaging::MsgPtrThief<MsgT> msg) {
    CallbackRawBaseSingle::sendMsg<MsgT>(msg);
  }

  template <typename T=void, typename=IsVoidType<MsgT,T>>
  void send() {
    CallbackRawBaseSingle::send();
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackRawBaseSingle::serialize(s);
  }
};

}}}} /* end namespace vt::pipe::callback::cbunion */

namespace vt {

using VoidMsg = pipe::signal::SigVoidType;

template <typename MsgT = VoidMsg>
using Callback = pipe::callback::cbunion::CallbackTyped<MsgT>;

using CallbackU = pipe::callback::cbunion::CallbackRawBaseSingle;

} /* end namespace vt */

#include "vt/pipe/callback/cb_union/cb_raw_base.impl.h"

#endif /*INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H*/
