
#if ! defined __RUNTIME_TRANSPORT_RDMA_INFO__
#define __RUNTIME_TRANSPORT_RDMA_INFO__

#include "common.h"
#include "rdma_common.h"

#include <unordered_map>
#include <vector>

namespace runtime { namespace rdma {

struct Info {
  using rdma_type_t = Type;

  byte_t num_bytes = no_byte;
  TagType tag = no_tag;
  rdma_type_t rdma_type;
  rdma_ptr_t data_ptr = no_rdma_ptr;
  rdma_continuation_t cont = no_action;
  action_t cont_action = no_action;
  byte_t offset = no_byte;

  Info(
    rdma_type_t const& in_rdma_type, byte_t const& in_num_bytes = no_byte,
    byte_t const& in_offset = no_byte, TagType const& in_tag = no_tag,
    rdma_continuation_t in_cont = no_action, action_t in_cont_action = no_action,
    rdma_ptr_t const& in_data_ptr = no_rdma_ptr
  ) : rdma_type(in_rdma_type), num_bytes(in_num_bytes), tag(in_tag),
      data_ptr(in_data_ptr), cont(in_cont), cont_action(in_cont_action),
      offset(in_offset)
  { }
};

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_INFO__*/
