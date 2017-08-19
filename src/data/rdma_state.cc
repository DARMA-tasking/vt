
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

}} //end namespace runtime::rdma
