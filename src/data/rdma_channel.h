
#if ! defined __RUNTIME_TRANSPORT_RDMA_CHANNEL__
#define __RUNTIME_TRANSPORT_RDMA_CHANNEL__

#include "common.h"
#include "function.h"
#include "rdma_common.h"
#include "rdma_handle.h"

#include <mpi.h>

#include <unordered_map>

namespace runtime { namespace rdma {

static constexpr byte_t const rdma_elm_size = sizeof(char);
static constexpr byte_t const rdma_empty_byte = 0;

struct Channel {
  using rdma_handle_manager_t = HandleManager;
  using rdma_type_t = Type;

  Channel(
    rdma_handle_t const& in_rdma_handle, rdma_type_t const& in_op_type,
    bool const& in_is_target, tag_t const& in_channel_group_tag,
    node_t const& in_non_target = uninitialized_destination,
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

  node_t
  get_target() const;

  node_t
  get_non_target() const;

private:
  void
  init_channel_window();

private:
  bool const is_target;

  bool initialized = false, locked = false, flushed = true;
  rdma_handle_t const rdma_handle = no_rdma_handle;
  node_t target = uninitialized_destination;
  node_t my_node = uninitialized_destination;
  node_t non_target = uninitialized_destination;
  byte_t num_bytes = no_byte;
  rdma_ptr_t ptr = no_rdma_ptr;
  rdma_type_t op_type = uninitialized_rdma_type;

  tag_t channel_group_tag = no_tag;

  MPI_Win window;
  MPI_Group channel_group;
  MPI_Comm channel_comm;
};

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_CHANNEL__*/
