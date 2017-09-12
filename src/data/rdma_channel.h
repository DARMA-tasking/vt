
#if ! defined __RUNTIME_TRANSPORT_RDMA_CHANNEL__
#define __RUNTIME_TRANSPORT_RDMA_CHANNEL__

#include "common.h"
#include "function.h"
#include "rdma_common.h"
#include "rdma_channel_lookup.h"
#include "rdma_handle.h"

#include <mpi.h>

#include <unordered_map>

namespace runtime { namespace rdma {

static constexpr byte_t const rdma_elm_size = sizeof(char);
static constexpr byte_t const rdma_empty_byte = 0;

struct Channel {
  using rdma_handle_manager_t = HandleManager;
  using rdma_type_t = Type;
  using rdma_group_pos_t = int;

  static constexpr rdma_group_pos_t const no_group_pos = -1;

  Channel(
    rdma_handle_t const& in_rdma_handle, rdma_type_t const& in_op_type,
    NodeType const& in_target, TagType const& in_channel_group_tag,
    NodeType const& in_non_target = uninitialized_destination,
    rdma_ptr_t const& in_ptr = nullptr, byte_t const& in_num_bytes = no_byte
  );

  void
  init_channel_group();

  void
  sync_channel_local();

  void
  sync_channel_global();

  void
  lock_channel_for_op();

  void
  unlock_channel_for_op();

  void
  write_data_to_channel(
    rdma_ptr_t const& ptr, byte_t const& ptr_num_bytes, byte_t const& offset
  );

  void
  free_channel();

  virtual ~Channel();

  NodeType
  get_target() const;

  NodeType
  get_non_target() const;

private:
  void
  init_channel_window();

private:
  bool is_target = false;

  bool initialized = false, locked = false, flushed = true;
  rdma_handle_t const rdma_handle = no_rdma_handle;
  rdma_group_pos_t target_pos = no_group_pos;
  rdma_group_pos_t non_target_pos = no_group_pos;
  NodeType target = uninitialized_destination;
  NodeType my_node = uninitialized_destination;
  NodeType non_target = uninitialized_destination;
  byte_t num_bytes = no_byte;
  rdma_ptr_t ptr = no_rdma_ptr;
  rdma_type_t op_type = uninitialized_rdma_type;

  TagType channel_group_tag = no_tag;

  MPI_Win window;
  MPI_Group channel_group;
  MPI_Comm channel_comm;
};

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_CHANNEL__*/
