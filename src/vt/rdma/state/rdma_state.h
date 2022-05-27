/*
//@HEADER
// *****************************************************************************
//
//                                 rdma_state.h
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

#if !defined INCLUDED_VT_RDMA_STATE_RDMA_STATE_H
#define INCLUDED_VT_RDMA_STATE_RDMA_STATE_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"
#include "vt/rdma/rdma_handle.h"
#include "vt/rdma/rdma_msg.h"
#include "vt/rdma/rdma_info.h"

#include "vt/rdma/group/rdma_map.h"
#include "vt/rdma/group/rdma_group.h"

#include "vt/rdma/channel/rdma_channel.h"

#include <unordered_map>
#include <vector>

namespace vt { namespace rdma {

static constexpr RDMA_HandlerType const first_rdma_handler = 1;

struct State {
  using RDMA_InfoType = Info;
  using RDMA_TypeType = Type;
  using RDMA_MapType = Map;
  using RDMA_GroupType = Group;
  using RDMA_GetFunctionType = ActiveGetFunctionType;
  using RDMA_PutFunctionType = ActivePutFunctionType;
  template <typename MsgType>
  using RDMA_GetTypedFunctionType = ActiveTypedGetFunctionType<MsgType>;
  template <typename MsgType>
  using RDMA_PutTypedFunctionType = ActiveTypedPutFunctionType<MsgType>;
  using RDMA_TagGetHolderType =
    std::tuple<RDMA_GetFunctionType, RDMA_HandlerType, vt::HandlerType>;
  using RDMA_TagPutHolderType =
    std::tuple<RDMA_PutFunctionType, RDMA_HandlerType, vt::HandlerType>;
  using RDMA_FunctionType = BaseMessage;

  template <typename T>
  using TagContainerType = std::unordered_map<TagType, T>;

  template <typename T>
  using ContainerType = std::vector<T>;

  RDMA_HandleType handle = no_rdma_handle;
  RDMA_PtrType ptr = no_rdma_ptr;
  ByteType num_bytes = no_byte;

  State(
    RDMA_HandleType const& in_handle, RDMA_PtrType const& in_ptr = no_rdma_ptr,
    ByteType const& in_num_bytes = no_byte,
    bool const& use_default_handler = false
  );

  template <typename MsgT, typename FuncT, ActiveTypedRDMAGetFnType<MsgT>* f>
  RDMA_HandlerType setRDMAGetFn(
    MsgT* msg, FuncT const& fn, bool const& any_tag = false,
    TagType const& tag = no_tag
  );

  template <typename MsgT, typename FuncT, ActiveTypedRDMAPutFnType<MsgT>* f>
  RDMA_HandlerType setRDMAPutFn(
    MsgT* msg, FuncT const& fn, bool const& any_tag = false,
    TagType const& tag = no_tag
  );

  void unregisterRdmaHandler(
    RDMA_TypeType const& type, TagType const& tag, bool const& use_default
  );

  void unregisterRdmaHandler(
    RDMA_HandlerType const& handler, TagType const& tag
  );

  RDMA_HandlerType makeRdmaHandler(RDMA_TypeType const& rdma_type);

  bool testReadyGetData(TagType const& tag);
  bool testReadyPutData(TagType const& tag);

  void getData(
    GetMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info,
    NodeType const& from_node
  );

  void putData(
    PutMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info,
    NodeType const& from_node
  );

  void processPendingGet(TagType const& tag = no_tag);
  void setDefaultHandler();

  static RDMA_GetType defaultGetHandlerFn(
    StateMessage<State>* msg, ByteType num_bytes, ByteType req_offset,
    TagType tag, bool is_local
  );

  static void defaultPutHandlerFn(
    StateMessage<State>* msg, RDMA_PtrType in_ptr, ByteType in_num_bytes,
    ByteType req_offset, TagType tag, bool is_local
  );

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | using_default_put_handler
      | using_default_get_handler
      | group_info
      | this_rdma_get_handler
      | this_rdma_put_handler
      | this_get_handler
      | this_put_handler
      | get_any_tag
      | put_any_tag
      | get_tag_holder
      | put_tag_holder
      | pending_tag_gets
      | pending_tag_puts
      | user_state_get_msg_
      | user_state_put_msg_
      | no_rdma_handle
      | no_rdma_ptr
      | no_byte;

    s.countBytes(rdma_get_fn);
    s.countBytes(rdma_put_fn);
  }

  bool using_default_put_handler = false;
  bool using_default_get_handler = false;

  std::unique_ptr<RDMA_GroupType> group_info = nullptr;

private:
  RDMA_HandlerType this_rdma_get_handler = uninitialized_rdma_handler;
  RDMA_HandlerType this_rdma_put_handler = uninitialized_rdma_handler;
  ::vt::HandlerType this_get_handler = uninitialized_handler;
  ::vt::HandlerType this_put_handler = uninitialized_handler;

  bool get_any_tag = false;
  bool put_any_tag = false;

  RDMA_GetFunctionType rdma_get_fn = no_action;
  RDMA_PutFunctionType rdma_put_fn = no_action;

  TagContainerType<RDMA_TagGetHolderType> get_tag_holder;
  TagContainerType<RDMA_TagPutHolderType> put_tag_holder;
  TagContainerType<ContainerType<RDMA_InfoType>> pending_tag_gets;
  TagContainerType<ContainerType<RDMA_InfoType>> pending_tag_puts;

  RDMA_FunctionType* user_state_get_msg_ = nullptr;
  RDMA_FunctionType* user_state_put_msg_ = nullptr;
};

}} //end namespace vt::rdma

#include "vt/rdma/state/rdma_state.impl.h"

#endif /*INCLUDED_VT_RDMA_STATE_RDMA_STATE_H*/
