
#include "common.h"
#include "rdma_state.h"
#include "transport.h"

namespace runtime { namespace rdma {

template<>
rdma_handler_t
RDMAState::set_rdma_fn<
  RDMAState::rdma_type_t::Get, RDMAState::rdma_get_function_t
>(rdma_get_function_t const& fn, tag_t const& tag) {
  rdma_handler_t const handler = make_rdma_handler(rdma_type_t::Get);
  if (tag == no_tag) {
    rdma_get_fn = fn;
  } else {
    get_tag_holder[tag] = fn;
  }
  return handler;
}

template<>
rdma_handler_t
RDMAState::set_rdma_fn<
  RDMAState::rdma_type_t::Put, RDMAState::rdma_put_function_t
>(rdma_put_function_t const& fn, tag_t const& tag) {
  rdma_handler_t const handler = make_rdma_handler(rdma_type_t::Put);
  if (tag == no_tag) {
    rdma_put_fn = fn;
  } else {
    put_tag_holder[tag] = fn;
  }
  return handler;
}

rdma_handler_t
RDMAState::make_rdma_handler(rdma_type_t const& rdma_type) {
  rdma_handler_t& handler =
    rdma_type == rdma_type_t::Put ? this_rdma_put_handler : this_rdma_get_handler;

  if (handler == uninitialized_rdma_handler) {
    handler = the_rdma->allocate_new_rdma_handler();
  }

  return handler;
}

bool
RDMAState::test_ready_get_data(tag_t const& tag) {
  bool const not_ready =
    ((tag == no_tag and rdma_get_fn == nullptr) or
     get_tag_holder.find(tag) == get_tag_holder.end());
  return not not_ready;
}

void
RDMAState::get_data(
  GetMessage* msg, bool const& is_user_msg, rdma_info_t const& info,
  rdma_continuation_t cont
) {
  auto const& tag = info.tag;

  assert(
    not is_user_msg and "User-level get messages currently unsupported"
  );

  bool const ready = test_ready_get_data(info.tag);

  if (ready) {
    rdma_get_function_t get_fn =
      tag == no_tag ? rdma_get_fn : get_tag_holder.find(tag)->second;
    cont(get_fn(nullptr, info.num_bytes, info.tag));
  } else {
    rdma_info_t new_info{info};
    new_info.cont = cont;
    pending_tag_gets[tag].push_back(new_info);
  }
}

void
RDMAState::process_pending_get(tag_t const& tag) {
  bool const ready = test_ready_get_data(tag);
  assert(ready and "Must be ready to process pending");

  rdma_get_function_t get_fn =
    tag == no_tag ? rdma_get_fn : get_tag_holder.find(tag)->second;

  auto pending_iter = pending_tag_gets.find(tag);
  if (pending_iter != pending_tag_gets.end()) {
    for (auto&& elm : pending_iter->second) {
      elm.cont(get_fn(nullptr, elm.num_bytes, elm.tag));
    }
  }
}


}} //end namespace runtime::rdma
