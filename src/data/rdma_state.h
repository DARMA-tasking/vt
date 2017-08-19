
#if ! defined __RUNTIME_TRANSPORT_RDMASTATE__
#define __RUNTIME_TRANSPORT_RDMASTATE__

#include "common.h"
#include "rdma_common.h"
#include "rdma_handle.h"
#include "rdma_msg.h"

#include <unordered_map>
#include <vector>

namespace runtime { namespace rdma {

static constexpr rdma_handler_t const first_rdma_handler = 1;

struct RDMAInfo {
  using rdma_type_t = RDMAType;

  byte_t num_bytes = no_byte;
  tag_t tag = no_tag;
  rdma_type_t rdma_type;

  rdma_continuation_t cont = nullptr;

  RDMAInfo(
    rdma_type_t const& in_rdma_type, byte_t const& in_num_bytes = no_byte,
    tag_t const& in_tag = no_tag, rdma_continuation_t in_cont = nullptr
  ) : rdma_type(in_rdma_type), num_bytes(in_num_bytes), tag(in_tag),
      cont(in_cont)
  { }
};

struct RDMAState {
  using rdma_info_t = RDMAInfo;
  using rdma_type_t = RDMAType;
  using rdma_get_function_t = active_get_function_t;
  using rdma_put_function_t = active_put_function_t;

  template <typename T>
  using tag_container_t = std::unordered_map<tag_t, T>;

  template <typename T>
  using container_t = std::vector<T>;

  rdma_handle_t handle;
  rdma_ptr_t ptr = nullptr;
  byte_t num_bytes = 0;

  RDMAState(
    rdma_handle_t const& in_handle, byte_t const& in_num_bytes
  ) : handle(in_handle), num_bytes(in_num_bytes)
  { }

  template <rdma_type_t rdma_type, typename FunctionT>
  rdma_handler_t
  set_rdma_fn(FunctionT const& fn, tag_t const& tag = no_tag);

  rdma_handler_t
  make_rdma_handler(rdma_type_t const& rdma_type);

  bool
  test_ready_get_data(tag_t const& tag);

  void
  get_data(
    GetMessage* msg, bool const& is_user_msg, rdma_info_t const& info
  );

  void
  process_pending_get(tag_t const& tag = no_tag);

private:
  rdma_handler_t this_rdma_get_handler = uninitialized_rdma_handler,
                 this_rdma_put_handler = uninitialized_rdma_handler;

  rdma_get_function_t rdma_get_fn = nullptr;
  rdma_put_function_t rdma_put_fn = nullptr;

  tag_container_t<rdma_get_function_t> get_tag_holder;
  tag_container_t<rdma_put_function_t> put_tag_holder;

  tag_container_t<container_t<rdma_info_t>> pending_tag_gets, pending_tag_puts;
};

template<>
rdma_handler_t
RDMAState::set_rdma_fn<
  RDMAState::rdma_type_t::Put, RDMAState::rdma_put_function_t
>(rdma_put_function_t const& fn, tag_t const& tag);

template<>
rdma_handler_t
RDMAState::set_rdma_fn<
  RDMAState::rdma_type_t::Get, RDMAState::rdma_get_function_t
>(rdma_get_function_t const& fn, tag_t const& tag);


}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMASTATE__*/
