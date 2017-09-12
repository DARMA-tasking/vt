
#if ! defined __RUNTIME_TRANSPORT_RDMA_COMMON__
#define __RUNTIME_TRANSPORT_RDMA_COMMON__

#include "common.h"
#include "message.h"
#include "bit_common.h"

#include "rdma_types.h"

#include <tuple>

namespace runtime { namespace rdma {

using RDMA_IdentifierType = int32_t;

static constexpr RDMA_IdentifierType const first_rdma_identifier = 1;
static constexpr RDMA_IdentifierType const uninitialized_rdma_identifier = -1;
static constexpr TagType const first_rdma_channel_tag = 1;

// 64 bits: RDMA handle
//   int64_t handle/handler : [24..56]
//   int64_t node : 16 [7..23]
//   int64_t op_type : 4 [3]
//   int64_t is_HandlerTypeype : 1 [2]
//   int64_t is_collective : 1 [1]
//   int64_t is_sized : 1 [0]

enum Type {
  Get = 0,
  Put = 1,
  GetOrPut = 2,
  Uninitialized = 3
};

static constexpr BitCountType const rdma_type_num_bits = 4;
static constexpr BitCountType const rdma_sized_num_bits = 1;
static constexpr BitCountType const rdma_collective_num_bits = 1;
static constexpr BitCountType const rdma_hander_type_num_bits = 1;
static constexpr BitCountType const rdma_identifier_num_bits =
  BitCounterType<RDMA_IdentifierType>::value;

enum Bits {
  Sized       = 0,
  Collective  = Bits::Sized       + rdma_sized_num_bits,
  HandlerType = Bits::Collective  + rdma_collective_num_bits,
  OpType      = Bits::HandlerType + rdma_hander_type_num_bits,
  Identifier  = Bits::OpType      + rdma_type_num_bits,
  Node        = Bits::Identifier  + rdma_identifier_num_bits
};

using RDMA_OpType = int64_t;

static constexpr RDMA_OpType const no_rdma_op = -1;

using ActiveGetFunctionType = std::function<RDMA_GetType(BaseMessage*, ByteType, ByteType, TagType)>;
using ActivePutFunctionType = std::function<void(BaseMessage*, RDMA_PtrType, ByteType, ByteType, TagType)>;

using RDMA_PtrContinuationType = std::function<void(RDMA_PtrType)>;
using RDMA_RecvType = std::function<void(void* ptr, size_t num_bytes)>;

using RDMA_NumElemsType = int64_t;
using RDMA_BlockElmRangeType = std::tuple<RDMA_BlockType,RDMA_ElmType,RDMA_ElmType>;
using RDMA_BlockMapType = std::function<NodeType(RDMA_BlockType,RDMA_BlockType)>;
using RDMA_ElmMapType = std::function<RDMA_BlockElmRangeType(RDMA_ElmType,RDMA_ElmType,RDMA_BlockType)>;

static constexpr Type uninitialized_rdma_type = Type::Uninitialized;
static constexpr ByteType rdma_default_byte_size = sizeof(char);

}} //end namespace runtime::rdma

#define print_channel_type(rdma_op_type) (                              \
  rdma_op_type == runtime::rdma::Type::Get ? "rdma::Get" : (            \
    rdma_op_type == runtime::rdma::Type::Put ? "rdma::Put" : (          \
      rdma_op_type == runtime::rdma::Type::GetOrPut ? "rdma::GetorPut"  \
      : (                                                               \
        rdma_op_type == runtime::rdma::Type::Uninitialized ?            \
        "rdma::Uninitialized" : "Error: unknown rdma::Type"             \
      )                                                                 \
    )                                                                   \
  )                                                                     \
)

#endif /*__RUNTIME_TRANSPORT_RDMA_COMMON__*/
