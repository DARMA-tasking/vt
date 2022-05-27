/*
//@HEADER
// *****************************************************************************
//
//                                rdma_state.cc
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

#include "vt/config.h"
#include "vt/rdma/rdma.h"
#include "vt/rdma/state/rdma_state.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/trace/trace_common.h"
#include "vt/messaging/active.h"

#include <cstring>
#include <memory>

namespace vt { namespace rdma {

State::State(
  RDMA_HandleType const& in_handle, RDMA_PtrType const& in_ptr,
  ByteType const& in_num_bytes, bool const& use_default_handler
) : handle(in_handle), ptr(in_ptr), num_bytes(in_num_bytes) {
  if (use_default_handler) {
    vtAssert(
      in_num_bytes != no_byte && ptr != nullptr,
      "Bytes, ptr must be set to use default"
    );
  }
}

void State::setDefaultHandler() {
  using StateMsgT = StateMessage<State>;

  bool const handle_any_tag = true;

  theRDMA()->associateGetFunction<StateMsgT,State::defaultGetHandlerFn>(
    nullptr, handle, State::defaultGetHandlerFn, handle_any_tag
  );
  using_default_get_handler = true;

  theRDMA()->associatePutFunction<StateMsgT,State::defaultPutHandlerFn>(
    nullptr, handle, State::defaultPutHandlerFn, handle_any_tag
  );
  using_default_put_handler = true;
}

void State::unregisterRdmaHandler(
  RDMA_TypeType const& type, TagType const& tag, bool const& use_default
) {
  if (type == RDMA_TypeType::Get or type == RDMA_TypeType::GetOrPut) {
    if (tag == no_tag or use_default) {
      this_rdma_get_handler = uninitialized_rdma_handler;
      rdma_get_fn = nullptr;
      using_default_get_handler = false;
    } else {
      auto iter = get_tag_holder.find(tag);
      if (iter != get_tag_holder.end()) {
        get_tag_holder.erase(iter);
      }
    }
  }
  if (type == RDMA_TypeType::Put or type == RDMA_TypeType::GetOrPut) {
    if (tag == no_tag or use_default) {
      this_rdma_put_handler = uninitialized_rdma_handler;
      rdma_put_fn = nullptr;
      using_default_put_handler = false;
    } else {
      auto iter = put_tag_holder.find(tag);
      if (iter != put_tag_holder.end()) {
        put_tag_holder.erase(iter);
      }
    }
  }
}

void State::unregisterRdmaHandler(
  RDMA_HandlerType const& handler, TagType const& tag
) {
  if (tag == no_tag) {
    if (this_rdma_get_handler == handler) {
      this_rdma_get_handler = uninitialized_rdma_handler;
      this_get_handler = uninitialized_handler;
      rdma_get_fn = nullptr;
      using_default_get_handler = false;
    }
    if (this_rdma_put_handler == handler) {
      this_rdma_put_handler = uninitialized_rdma_handler;
      this_put_handler = uninitialized_handler;
      rdma_put_fn = nullptr;
      using_default_put_handler = false;
    }
  } else {
    auto p_get_iter = get_tag_holder.find(tag);
    if (p_get_iter != get_tag_holder.end() and
        std::get<1>(p_get_iter->second) == handler) {
      get_tag_holder.erase(p_get_iter);
    }

    auto p_put_iter = put_tag_holder.find(tag);
    if (p_put_iter != put_tag_holder.end() and
        std::get<1>(p_get_iter->second) == handler) {
      put_tag_holder.erase(p_put_iter);
    }
  }
}

RDMA_HandlerType State::makeRdmaHandler(RDMA_TypeType const& rdma_type) {
  RDMA_HandlerType& handler =
    rdma_type == RDMA_TypeType::Put ? this_rdma_put_handler : this_rdma_get_handler;

  if (handler == uninitialized_rdma_handler) {
    handler = theRDMA()->allocateNewRdmaHandler();
  }

  return handler;
}

bool State::testReadyGetData(TagType const& tag) {
  bool const not_ready = (
    ((tag == no_tag or get_any_tag) and rdma_get_fn == nullptr) or (
      tag != no_tag and
      not get_any_tag and
      get_tag_holder.find(tag) == get_tag_holder.end()
    )
  );
  return not not_ready;
}

bool State::testReadyPutData(TagType const& tag) {
  bool const not_ready = (
    ((tag == no_tag or put_any_tag) and rdma_put_fn == nullptr) or (
      tag != no_tag and
      not get_any_tag and
      put_tag_holder.find(tag) == put_tag_holder.end()
    )
  );
  return not not_ready;
}

/*static*/ RDMA_GetType State::defaultGetHandlerFn(
  StateMessage<State>* msg, ByteType req_num_bytes, ByteType req_offset,
  TagType tag, bool is_local
) {
  auto const& state = *msg->state;

  vt_debug_print(
    normal, rdma_state,
    "defaultGetHandlerFn: msg={}, req_num_bytes={}, tag={}\n",
    print_ptr(msg), req_num_bytes, tag
  );

  vtAssert(
    state.ptr != nullptr && state.num_bytes != no_byte,
    "To use default handler ptr, bytes must be set"
  );

  return RDMA_GetType{
    static_cast<char*>(state.ptr) + req_offset,
    req_num_bytes == no_byte ? state.num_bytes : req_num_bytes
  };
}

/*static*/ void State::defaultPutHandlerFn(
  StateMessage<State>* msg, RDMA_PtrType in_ptr, ByteType req_num_bytes,
  ByteType req_offset, TagType tag, bool is_local
) {
  auto const& state = *msg->state;

  vt_debug_print(
    normal, rdma_state,
    "defaultPutHandlerFn: msg={}, ptr={}, req_num_bytes={}, tag={}\n",
    print_ptr(msg), print_ptr(in_ptr), req_num_bytes, tag
  );

  vtAssert(
    state.ptr != nullptr && state.num_bytes != no_byte,
    "To use default handler ptr, bytes must be set"
  );

  std::memcpy(static_cast<char*>(state.ptr) + req_offset, in_ptr, req_num_bytes);
}

void State::getData(
  GetMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info,
  NodeType const& from_node
) {
  auto const& tag = info.tag;

  vtAssert(
    not is_user_msg, "User-level get messages currently unsupported"
  );

  bool const ready = testReadyGetData(info.tag);

  vt_debug_print(
    normal, rdma_state,
    "getData: msg={}, tag={}, ready={}, handle={}, get_any_tag={},"
    " is_local={}\n",
    print_ptr(msg), info.tag, print_bool(ready), handle,
    print_bool(get_any_tag), print_bool(info.is_local)
  );

  auto const& offset = info.offset;

  StateMessage<State> state_msg(this);
  BaseMessage* base_msg =
    user_state_get_msg_ ? user_state_get_msg_ : &state_msg;

  if (ready) {
    RDMA_GetFunctionType get_fn =
      tag == no_tag or get_any_tag ? rdma_get_fn :
      std::get<0>(get_tag_holder.find(tag)->second);

#if vt_check_enabled(trace_enabled)
    trace::TraceProcessingTag processing_tag;
    {
      ::vt::HandlerType const reg_han =
        tag == no_tag or get_any_tag ? this_get_handler :
        std::get<2>(get_tag_holder.find(tag)->second);
      trace::TraceEntryIDType trace_id = auto_registry::handlerTraceID(reg_han);
      trace::TraceEventIDType event = theContext()->getTraceEventCurrentTask();
      size_t msg_size = info.num_bytes;

      processing_tag =
        theTrace()->beginProcessing(trace_id, msg_size, event, from_node);
    }
#endif

    if (info.cont) {
      info.cont(
        get_fn(base_msg, info.num_bytes, offset, info.tag, info.is_local)
      );
    } else if (info.data_ptr) {
      RDMA_GetType get = get_fn(
        base_msg, info.num_bytes, offset, info.tag, info.is_local
      );
      memcpy(info.data_ptr, std::get<0>(get), std::get<1>(get));
      if (info.cont_action) {
        info.cont_action();
      }
    }

#if vt_check_enabled(trace_enabled)
    theTrace()->endProcessing(processing_tag);
#endif
  } else {
    pending_tag_gets[tag].push_back(info);
  }
}

void State::putData(
  PutMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info,
  NodeType const& from_node
) {
  auto const& tag = info.tag;

  vtAssert(
    not is_user_msg, "User-level get messages currently unsupported"
  );

  bool const ready = testReadyPutData(info.tag);

  vt_debug_print(
    normal, rdma_state,
    "putData: msg={}, tag={}, ptr={}, num_bytes={}, "
    "ready={}, handle={}, get_any_tag={}, is_local={}\n",
    print_ptr(msg), info.tag, print_ptr(info.data_ptr), info.num_bytes,
    print_bool(ready), handle, print_bool(get_any_tag),
    print_bool(info.is_local)
  );

  StateMessage<State> state_msg(this);
  BaseMessage* const base_msg =
    user_state_put_msg_ ? user_state_put_msg_ : &state_msg;

  if (ready) {
    RDMA_PutFunctionType put_fn =
      tag == no_tag or put_any_tag ? rdma_put_fn :
      std::get<0>(put_tag_holder.find(tag)->second);

#if vt_check_enabled(trace_enabled)
    trace::TraceProcessingTag processing_tag;
    {
      ::vt::HandlerType const reg_han =
        tag == no_tag or put_any_tag ? this_put_handler :
        std::get<2>(put_tag_holder.find(tag)->second);
      trace::TraceEntryIDType trace_id = auto_registry::handlerTraceID(reg_han);
      trace::TraceEventIDType event = theContext()->getTraceEventCurrentTask();
      size_t msg_size = info.num_bytes;

      processing_tag =
        theTrace()->beginProcessing(trace_id, msg_size, event, from_node);
    }
#endif

    put_fn(
      base_msg, info.data_ptr, info.num_bytes, info.offset, info.tag,
      info.is_local
    );

    vtAssert(
      not info.cont, "Cont should not be set for a put"
    );

    if (info.cont_action) {
      info.cont_action();
    }

#if vt_check_enabled(trace_enabled)
    theTrace()->endProcessing(processing_tag);
#endif
  } else {
    pending_tag_puts[tag].push_back(info);
  }
}

void State::processPendingGet(TagType const& tag) {
  bool const ready = testReadyGetData(tag);
  vtAssert(ready, "Must be ready to process pending");

  RDMA_GetFunctionType get_fn =
    tag == no_tag ? rdma_get_fn : std::get<0>(get_tag_holder.find(tag)->second);

  StateMessage<State> state_msg(this);

  auto pending_iter = pending_tag_gets.find(tag);
  if (pending_iter != pending_tag_gets.end()) {
    for (auto&& elm : pending_iter->second) {
      elm.cont(
        get_fn(&state_msg, elm.num_bytes, elm.offset, elm.tag, elm.is_local)
      );
    }
  }
}

// void
// State::set_collective_map(rdma_map_t const& map) {
//   vtAssert(
//     RDMA_HandleManagerType::is_collective(handle), "Must be collective handle"
//   );

//   collective_map = map;
// }

// rdma_map_t
// State::get_collective_map() {
//   return collective_map;
// }


// NodeType
// State::getNode(RDMA_ElmType const& elm) {
//   vtAssert(
//     collective_map != vtAullptr, RDMA_HandleManagerType::is_collective(handle)
//    , "Must be collective, have map assigned"
//   );

//   return collective_map(elm);
// }

}} //end namespace vt::rdma
