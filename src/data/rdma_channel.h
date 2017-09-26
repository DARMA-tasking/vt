
#if ! defined __RUNTIME_TRANSPORT_RDMA_CHANNEL__
#define __RUNTIME_TRANSPORT_RDMA_CHANNEL__

#include "config.h"
#include "registry_function.h"
#include "rdma_common.h"
#include "rdma_channel_lookup.h"
#include "rdma_handle.h"

#include <mpi.h>

#include <unordered_map>

namespace vt { namespace rdma {

constexpr ByteType const rdma_elm_size = sizeof(char);
constexpr ByteType const rdma_empty_byte = 0;

struct Channel {
  using RDMA_HandleManagerType = HandleManager;
  using RDMA_TypeType = Type;
  using RDMA_GroupPosType = int;

  static constexpr RDMA_GroupPosType const no_group_pos = -1;

  Channel(
    RDMA_HandleType const& in_rdma_handle, RDMA_TypeType const& in_op_type,
    NodeType const& in_target, TagType const& in_channel_group_tag,
    NodeType const& in_non_target = uninitialized_destination,
    RDMA_PtrType const& in_ptr = nullptr, ByteType const& in_num_bytes = no_byte
  );

  virtual ~Channel();

  void initChannelGroup();
  void syncChannelLocal();
  void syncChannelGlobal();
  void lockChannelForOp();
  void unlockChannelForOp();
  void freeChannel();

  void writeDataToChannel(
    RDMA_PtrType const& ptr, ByteType const& ptr_num_bytes, ByteType const& offset
  );

  NodeType getTarget() const;
  NodeType getNonTarget() const;

private:
  void initChannelWindow();

private:
  bool is_target_ = false;

  bool initialized_ = false, locked_ = false, flushed_ = true;
  RDMA_HandleType const rdma_handle_ = no_rdma_handle;
  RDMA_GroupPosType target_pos_ = no_group_pos;
  RDMA_GroupPosType non_target_pos_ = no_group_pos;
  NodeType target_ = uninitialized_destination;
  NodeType my_node_ = uninitialized_destination;
  NodeType non_target_ = uninitialized_destination;
  ByteType num_bytes_ = no_byte;
  RDMA_PtrType ptr_ = no_rdma_ptr;
  RDMA_TypeType op_type_ = uninitialized_rdma_type;

  TagType channel_group_tag_ = no_tag;

  MPI_Win window_;
  MPI_Group channel_group_;
  MPI_Comm channel_comm_;
};

}} //end namespace vt::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_CHANNEL__*/
