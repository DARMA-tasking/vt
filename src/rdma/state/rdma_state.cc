
#include "config.h"
#include "rdma/rdma.h"
#include "rdma/state/rdma_state.h"

#include <cstring>
#include <memory>

namespace vt { namespace rdma {

State::State(
  RDMA_HandleType const& in_handle, RDMA_PtrType const& in_ptr,
  ByteType const& in_num_bytes, bool const& use_default_handler
) : handle(in_handle), ptr(in_ptr), num_bytes(in_num_bytes) {
  if (use_default_handler) {
    assert(
      in_num_bytes != no_byte and ptr != nullptr and
      "Bytes and ptr must be set to use default"
    );
  }
}

void State::setDefaultHandler() {
  using StateMsgT = StateMessage<State>;

  bool const handle_any_tag = true;

  theRDMA()->associateGetFunction<StateMsgT>(
    nullptr, handle, State::defaultGetHandlerFn, handle_any_tag
  );
  using_default_get_handler = true;

  theRDMA()->associatePutFunction<StateMsgT>(
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
      rdma_get_fn = nullptr;
      using_default_get_handler = false;
    }
    if (this_rdma_put_handler == handler) {
      this_rdma_put_handler = uninitialized_rdma_handler;
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

  debug_print(
    rdma_state, node,
    "defaultGetHandlerFn: msg=%p, req_num_bytes=%lld, tag=%d\n",
    msg, req_num_bytes, tag
  );

  assert(
    state.ptr != nullptr and state.num_bytes != no_byte and
    "To use default handler ptr and bytes must be set"
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

  debug_print(
    rdma_state, node,
    "defaultPutHandlerFn: msg=%p, ptr=%p, req_num_bytes=%lld, tag=%d\n",
    msg, in_ptr, req_num_bytes, tag
  );

  assert(
    state.ptr != nullptr and state.num_bytes != no_byte and
    "To use default handler ptr and bytes must be set"
  );

  std::memcpy(static_cast<char*>(state.ptr) + req_offset, in_ptr, req_num_bytes);
}

void State::getData(
  GetMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info
) {
  auto const& tag = info.tag;

  assert(
    not is_user_msg and "User-level get messages currently unsupported"
  );

  bool const ready = testReadyGetData(info.tag);

  debug_print(
    rdma_state, node,
    "getData: msg=%p, tag=%d, ready=%s, handle=%lld, get_any_tag=%s,"
    " is_local=%s\n",
    msg, info.tag, print_bool(ready), handle, print_bool(get_any_tag),
    print_bool(info.is_local)
  );

  auto const& offset = info.offset;

  StateMessage<State> state_msg(this);
  BaseMessage* base_msg =
    user_state_get_msg_ ? user_state_get_msg_ : &state_msg;

  if (ready) {
    RDMA_GetFunctionType get_fn =
      tag == no_tag or get_any_tag ? rdma_get_fn :
      std::get<0>(get_tag_holder.find(tag)->second);
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
  } else {
    pending_tag_gets[tag].push_back(info);
  }
}

void State::putData(
  PutMessage* msg, bool const& is_user_msg, RDMA_InfoType const& info
) {
  auto const& tag = info.tag;

  assert(
    not is_user_msg and "User-level get messages currently unsupported"
  );

  bool const ready = testReadyPutData(info.tag);

  debug_print(
    rdma_state, node,
    "putData: msg=%p, tag=%d, ptr=%p, num_bytes=%lld, "
    "ready=%s, handle=%lld, get_any_tag=%s, is_local=%s\n",
    msg, info.tag, info.data_ptr, info.num_bytes, print_bool(ready),
    handle, print_bool(get_any_tag), print_bool(info.is_local)
  );

  StateMessage<State> state_msg(this);
  BaseMessage* const base_msg =
    user_state_put_msg_ ? user_state_put_msg_ : &state_msg;

  if (ready) {
    RDMA_PutFunctionType put_fn =
      tag == no_tag or put_any_tag ? rdma_put_fn :
      std::get<0>(put_tag_holder.find(tag)->second);
    put_fn(
      base_msg, info.data_ptr, info.num_bytes, info.offset, info.tag,
      info.is_local
    );

    assert(
      not info.cont and "Cont should not be set for a put"
    );

    if (info.cont_action) {
      info.cont_action();
    }
  } else {
    pending_tag_puts[tag].push_back(info);
  }
}

void State::processPendingGet(TagType const& tag) {
  bool const ready = testReadyGetData(tag);
  assert(ready and "Must be ready to process pending");

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
//   assert(
//     RDMA_HandleManagerType::is_collective(handle) and "Must be collective handle"
//   );

//   collective_map = map;
// }

// rdma_map_t
// State::get_collective_map() {
//   return collective_map;
// }


// NodeType
// State::getNode(RDMA_ElmType const& elm) {
//   assert(
//     collective_map != nullptr and RDMA_HandleManagerType::is_collective(handle)
//     and "Must be collective and have map assigned"
//   );

//   return collective_map(elm);
// }

}} //end namespace vt::rdma
