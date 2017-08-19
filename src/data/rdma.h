
#if ! defined __RUNTIME_TRANSPORT_RDMA__
#define __RUNTIME_TRANSPORT_RDMA__

#include "common.h"
#include "rdma_common.h"
#include "rdma_state.h"
#include "rdma_handle.h"

#include <unordered_map>

namespace runtime { namespace rdma {

struct RDMAManager {
  using rdma_bits_t = RDMABits;
  using rdma_state_t = RDMAState;
  using rdma_type_t = RDMAType;
  using rdma_container_t = std::unordered_map<rdma_handle_t, rdma_state_t>;
  using rdma_handle_manager_t = RDMAHandleManager;
  using rdma_get_function_t = rdma_state_t::rdma_get_function_t;
  using rdma_put_function_t = rdma_state_t::rdma_put_function_t;

  // void
  // put_data(rdma_ptr_t* ptr, rdma_handler_t const& handle) {
  // }

  rdma_handle_t
  register_new_rdma_handler(
    byte_t const& num_bytes = no_byte, bool const& is_collective = false
  );

  template <rdma_type_t rdma_type, typename FunctionT>
  rdma_handler_t
  associate_rdma_function(
    rdma_handle_t const& handler, FunctionT const& get_fn,
    tag_t const& assoicated_tag
  );

  rdma_handler_t
  associate_get_function(
    rdma_handle_t const& han, rdma_get_function_t const& fn,
    tag_t const& tag = no_tag
  ) {
    return associate_rdma_function<rdma_type_t::Get>(han, fn, tag);
  }

  rdma_handler_t
  associate_put_function(
    rdma_handle_t const& han, rdma_get_function_t const& fn,
    tag_t const& tag = no_tag
  ) {
    return associate_rdma_function<rdma_type_t::Put>(han, fn, tag);
  }

  // rdma_handle_t
  // register_new_rdma_ptr(rdma_ptr_t const& ptr);

  // rdma_ptr_t
  // get_rdma_ptr(rdma_handle_t const& handle);

  rdma_handler_t
  allocate_new_rdma_handler();

  static void
  register_all_rdma_handlers();

private:
  rdma_handler_t cur_rdma_handler = first_rdma_handler;

  rdma_identifier_t cur_ident = first_rdma_identifier;

  rdma_identifier_t cur_col_handle = first_rdma_identifier;

  rdma_container_t holder;
};

}} //end namespace runtime::rdma

namespace runtime {

extern std::unique_ptr<rdma::RDMAManager> the_rdma;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_RDMA__*/
