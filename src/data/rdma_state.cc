
#include "common.h"
#include "rdma_state.h"
#include "transport.h"

namespace runtime { namespace rdma {

State::State(
  rdma_handle_t const& in_handle,
  rdma_ptr_t const& in_ptr,
  byte_t const& in_num_bytes,
  bool const& use_default_handler
) : handle(in_handle), ptr(in_ptr), num_bytes(in_num_bytes) {
  if (use_default_handler) {
    assert(
      in_num_bytes != no_byte and ptr != nullptr and
      "Bytes and ptr must be set to use default"
    );
  }
}

void
State::set_default_handler() {
  using namespace std::placeholders;

  bool const handle_any_tag = true;

  auto f_get = std::bind(&State::default_get_handler_fn, this, _1, _2, _3);
  the_rdma->associate_get_function(handle, f_get, handle_any_tag);
  using_default_get_handler = true;

  auto f_put = std::bind(&State::default_put_handler_fn, this, _1, _2, _3, _4);
  the_rdma->associate_put_function(handle, f_put, handle_any_tag);
  using_default_put_handler = true;
}

void
State::unregister_rdma_handler(
  rdma_type_t const& type, tag_t const& tag, bool const& use_default
) {
  if (type == rdma_type_t::Get or type == rdma_type_t::GetOrPut) {
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
  if (type == rdma_type_t::Put or type == rdma_type_t::GetOrPut) {
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

void
State::unregister_rdma_handler(
  rdma_handler_t const& handler, tag_t const& tag
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

template<>
rdma_handler_t
State::set_rdma_fn<
  State::rdma_type_t::Get, State::rdma_get_function_t
>(rdma_get_function_t const& fn, bool const& any_tag, tag_t const& tag) {

  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "%d: set_rdma_fn: GET tag=%d, handle=%lld, any_tag=%s\n",
    this_node, tag, handle, print_bool(any_tag)
  );

  rdma_handler_t const handler = make_rdma_handler(rdma_type_t::Get);

  if (any_tag) {
    assert(
      tag == no_tag and "If any tag, you must not have a tag set"
    );
  }

  this_rdma_get_handler = handler;

  if (tag == no_tag) {
    rdma_get_fn = fn;
    get_any_tag = any_tag;
  } else {
    get_tag_holder[tag] = rdma_tag_get_holder_t{fn,handler};
  }

  return handler;
}

template<>
rdma_handler_t
State::set_rdma_fn<
  State::rdma_type_t::Put, State::rdma_put_function_t
>(rdma_put_function_t const& fn, bool const& any_tag, tag_t const& tag) {
  rdma_handler_t const handler = make_rdma_handler(rdma_type_t::Put);

  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "%d: set_rdma_fn: PUT tag=%d, handle=%lld, any_tag=%s\n",
    this_node, tag, handle, print_bool(any_tag)
  );

  if (any_tag) {
    assert(
      tag == no_tag and "If any tag, you must not have a tag set"
    );
  }

  this_rdma_put_handler = handler;

  if (tag == no_tag) {
    rdma_put_fn = fn;
    put_any_tag = any_tag;
  } else {
    put_tag_holder[tag] = rdma_tag_put_holder_t{fn,handler};
  }

  return handler;
}

rdma_handler_t
State::make_rdma_handler(rdma_type_t const& rdma_type) {
  rdma_handler_t& handler =
    rdma_type == rdma_type_t::Put ? this_rdma_put_handler : this_rdma_get_handler;

  if (handler == uninitialized_rdma_handler) {
    handler = the_rdma->allocate_new_rdma_handler();
  }

  return handler;
}

bool
State::test_ready_get_data(tag_t const& tag) {
  bool const not_ready = (
    ((tag == no_tag or get_any_tag) and rdma_get_fn == nullptr) or (
      tag != no_tag and
      not get_any_tag and
      get_tag_holder.find(tag) == get_tag_holder.end()
    )
  );
  return not not_ready;
}

bool
State::test_ready_put_data(tag_t const& tag) {
  bool const not_ready = (
    ((tag == no_tag or put_any_tag) and rdma_put_fn == nullptr) or (
      tag != no_tag and
      not get_any_tag and
      put_tag_holder.find(tag) == put_tag_holder.end()
    )
  );
  return not not_ready;
}

rdma_get_t
State::default_get_handler_fn(
  BaseMessage* msg, byte_t req_num_bytes, tag_t tag
) {
  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "%d: default_get_handler_fn: msg=%p, req_num_bytes=%lld, tag=%d\n",
    this_node, msg, req_num_bytes, tag
  );

  assert(
    ptr != nullptr and num_bytes != no_byte and
    "To use default handler ptr and bytes must be set"
  );

  return rdma_get_t{ptr, req_num_bytes == no_byte ? num_bytes : req_num_bytes};
}

void
State::default_put_handler_fn(
  BaseMessage* msg, rdma_ptr_t in_ptr, byte_t req_num_bytes, tag_t tag
) {
  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "%d: default_put_handler_fn: msg=%p, ptr=%p, req_num_bytes=%lld, tag=%d\n",
    this_node, msg, ptr, req_num_bytes, tag
  );

  assert(
    ptr != nullptr and num_bytes != no_byte and
    "To use default handler ptr and bytes must be set"
  );

  std::memcpy(ptr, in_ptr, req_num_bytes);
}

void
State::get_data(
  GetMessage* msg, bool const& is_user_msg, rdma_info_t const& info
) {
  auto const& tag = info.tag;

  assert(
    not is_user_msg and "User-level get messages currently unsupported"
  );

  bool const ready = test_ready_get_data(info.tag);

  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "%d: get_data: msg=%p, tag=%d, ready=%s, handle=%lld, get_any_tag=%s\n",
    this_node, msg, info.tag, print_bool(ready), handle, print_bool(get_any_tag)
  );

  if (ready) {
    rdma_get_function_t get_fn =
      tag == no_tag or get_any_tag ? rdma_get_fn :
      std::get<0>(get_tag_holder.find(tag)->second);
    if (info.cont) {
      info.cont(get_fn(nullptr, info.num_bytes, info.tag));
    } else if (info.data_ptr) {
      rdma_get_t get = get_fn(nullptr, info.num_bytes, info.tag);
      memcpy(info.data_ptr, std::get<0>(get), std::get<1>(get));
      if (info.cont_action) {
        info.cont_action();
      }
    }
  } else {
    pending_tag_gets[tag].push_back(info);
  }
}

void
State::put_data(
  PutMessage* msg, bool const& is_user_msg, rdma_info_t const& info
) {
  auto const& tag = info.tag;

  assert(
    not is_user_msg and "User-level get messages currently unsupported"
  );

  bool const ready = test_ready_put_data(info.tag);

  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "%d: put_data: msg=%p, tag=%d, ptr=%p, num_bytes=%lld, "
    "ready=%s, handle=%lld, get_any_tag=%s\n",
    this_node, msg, info.tag, info.data_ptr, info.num_bytes, print_bool(ready),
    handle, print_bool(get_any_tag)
  );

  if (ready) {
    rdma_put_function_t put_fn =
      tag == no_tag or put_any_tag ? rdma_put_fn :
      std::get<0>(put_tag_holder.find(tag)->second);
    put_fn(nullptr, info.data_ptr, info.num_bytes, info.tag);

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

void
State::process_pending_get(tag_t const& tag) {
  bool const ready = test_ready_get_data(tag);
  assert(ready and "Must be ready to process pending");

  rdma_get_function_t get_fn =
    tag == no_tag ? rdma_get_fn : std::get<0>(get_tag_holder.find(tag)->second);

  auto pending_iter = pending_tag_gets.find(tag);
  if (pending_iter != pending_tag_gets.end()) {
    for (auto&& elm : pending_iter->second) {
      elm.cont(get_fn(nullptr, elm.num_bytes, elm.tag));
    }
  }
}


}} //end namespace runtime::rdma
