
#if ! defined __RUNTIME_TRANSPORT_RDMA_COMMON__
#define __RUNTIME_TRANSPORT_RDMA_COMMON__

#include "common.h"
#include "message.h"

#include <tuple>

namespace runtime { namespace rdma {

using rdma_identifier_t = int32_t;

static constexpr rdma_identifier_t const first_rdma_identifier = 1;
static constexpr rdma_identifier_t const uninitialized_rdma_identifier = -1;
static constexpr tag_t const first_rdma_channel_tag = 1;

// 64 bits: RDMA handle
//   int64_t handle/handler : [20..52]
//   int64_t node : 16 [4..19]
//   int64_t op_type : 1 [3]
//   int64_t is_handler_type : 1 [2]
//   int64_t is_collective : 1 [1]
//   int64_t is_sized : 1 [0]

enum Type {
  Get = 0,
  Put = 1,
  GetOrPut = 2,
  Uninitialized = 3
};

static constexpr bit_count_t const rdma_type_num_bits = 3;
static constexpr bit_count_t const rdma_identifier_num_bits = sizeof(rdma_identifier_t)*8;
static constexpr bit_count_t const rdma_sized_num_bits = 1;
static constexpr bit_count_t const rdma_collective_num_bits = 1;
static constexpr bit_count_t const rdma_hander_type_num_bits = 1;
static constexpr bit_count_t const rdma_op_type_num_bits = 1;

enum Bits {
  Sized       = 0,
  Collective  = Bits::Sized       + rdma_sized_num_bits,
  HandlerType = Bits::Collective  + rdma_collective_num_bits,
  OpType      = Bits::HandlerType + rdma_hander_type_num_bits,
  Identifier  = Bits::OpType      + rdma_type_num_bits,
  Node        = Bits::Identifier  + rdma_identifier_num_bits
};

using rdma_op_t = int64_t;

static constexpr rdma_op_t const no_rdma_op = -1;

using active_get_function_t = std::function<rdma_get_t(BaseMessage*, byte_t, tag_t)>;
using active_put_function_t = std::function<void(BaseMessage*, rdma_ptr_t, byte_t, tag_t)>;

using rdma_ptr_continuation_t = std::function<void(rdma_ptr_t)>;
using rdma_recv_t = std::function<void(void* ptr, size_t num_bytes)>;

using rdma_num_elems_t = int64_t;
using rdma_elm_t = int64_t;
using rdma_collective_map_t = std::function<node_t(rdma_elm_t)>;

static constexpr Type uninitialized_rdma_type = Type::Uninitialized;

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_COMMON__*/
