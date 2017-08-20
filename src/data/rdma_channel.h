
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
  ) : rdma_handle(in_rdma_handle), op_type(in_op_type),is_target(in_is_target),
      channel_group_tag(in_channel_group_tag), non_target(in_non_target),
      ptr(in_ptr), num_bytes(in_num_bytes)
  {
    target = rdma_handle_manager_t::get_rdma_node(rdma_handle);
    my_node = the_context->get_node();

    assert(
      (target != my_node or is_target) and
      "A RDMA Channel can not be created within the same node"
    );

    if (target != my_node) {
      non_target = my_node;
    } else {
      assert(
        non_target != uninitialized_destination and
        "Target must know other destination (non-target)"
      );
    }

    if (in_num_bytes == no_byte) {
      num_bytes = rdma_empty_byte;
    }

    debug_print_rdma_channel(
      "%d: channel: construct: target=%d, non_target=%d, my_node=%d, han=%lld, "
      "ptr=%p, bytes=%lld, is_target=%s\n",
      my_node, target, non_target, my_node, rdma_handle, ptr, num_bytes,
      print_bool(is_target)
    );
  }

  void
  init_channel_group() {
    debug_print_rdma_channel(
      "%d: channel: init_channel_group: target=%d, my_node=%d, han=%lld\n",
      my_node, target, my_node, rdma_handle
    );

    MPI_Group world;
    auto const& group_create_ret = MPI_Comm_group(MPI_COMM_WORLD, &world);

    assert(
      group_create_ret == MPI_SUCCESS and
      "MPI_Comm_group: Should be successful"
    );

    int const channel_group_nodes[2] = {target, non_target};
    auto const& group_incl_ret = MPI_Group_incl(
      world, 2, channel_group_nodes, &channel_group
    );

    assert(
      group_incl_ret == MPI_SUCCESS and "MPI_Group_incl: Should be successful"
    );

    auto const& comm_create_ret = MPI_Comm_create_group(
      MPI_COMM_WORLD, channel_group, channel_group_tag, &channel_comm
    );

    assert(
      comm_create_ret == MPI_SUCCESS and
      "MPI_Comm_create_group: Should be successful"
    );

    debug_print_rdma_channel(
      "%d: channel: init_channel_group: finished\n", my_node
    );

    init_channel_window();
  }

  void
  sync_channel_local() {
    printf(
      "%d: channel: sync_channel_local: target=%d, locked=%s\n",
      my_node, target, print_bool(locked)
    );

    if (not locked) {
      lock_channel_for_op();
    }

    auto const& ret = MPI_Win_flush_local(0,window);
    assert(ret == MPI_SUCCESS and "MPI_Win_flush_local: Should be successful");

    if (op_type == rdma_type_t::Put) {
      unlock_channel_for_op();
    }
  }

  void
  sync_channel_global() {
    printf(
      "%d: channel: sync_channel_global: target=%d\n", my_node, target
    );

    auto const& ret = MPI_Win_flush(target, window);
    assert(ret == MPI_SUCCESS and "MPI_Win_flush: Should be successful");
  }

  void
  lock_channel_for_op() {
    assert(initialized and "Channel must be initialized");
    assert(not target and "The target can not write to this channel");

    if (not locked) {
      constexpr int const mpi_win_lock_assert_arg = 0;

      auto const& lock_type =
        op_type == rdma_type_t::Put ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED;

      debug_print_rdma_channel(
        "%d: lock_channel_for_op: is_target=%s, target=%d, op_type=%s, "
        "lock_type=%d, exclusive=%d\n",
        my_node, print_bool(is_target), target,
        op_type == rdma_type_t::Get ? "GET" : "PUT", lock_type, MPI_LOCK_EXCLUSIVE
      );

      auto const ret = MPI_Win_lock(
        lock_type, target, mpi_win_lock_assert_arg, window
      );

      assert(ret == MPI_SUCCESS and "MPI_Win_lock: Should be successful");

      locked = true;
    }
  }

  void
  unlock_channel_for_op() {
    assert(initialized and "Channel must be initialized");
    assert(not target and "The target can not write to this channel");

    if (locked) {
      auto const& ret = MPI_Win_unlock(target, window);

      assert(ret == MPI_SUCCESS and "MPI_Win_unlock: Should be successful");

      debug_print_rdma_channel(
        "%d: unlock_channel_for_op: target=%d, op_type=%s\n",
        my_node, target, op_type == rdma_type_t::Get ? "GET" : "PUT"
      );

      locked = false;
    }
  }

  void
  write_data_to_channel(
    rdma_ptr_t const& ptr, byte_t const& ptr_num_bytes, byte_t const& offset
  ) {
    assert(initialized and "Channel must be initialized");
    assert(not target and "The target can not write to this channel");

    byte_t const d_offset = offset == no_byte ? 0 : offset;

    debug_print_rdma_channel(
      "%d: write_data_to_channel: target=%d, ptr=%p, ptr_num_bytes=%lld, "
      "num_bytes=%lld, op_type=%s, offset=%lld\n",
      my_node, target, ptr, ptr_num_bytes, num_bytes,
      op_type == rdma_type_t::Get ? "GET" : "PUT", offset
    );

    if (not locked) {
      lock_channel_for_op();
    }

    if (op_type == rdma_type_t::Get) {
      auto const& get_ret = MPI_Get(
        ptr, ptr_num_bytes, MPI_BYTE, target, d_offset, num_bytes, MPI_BYTE, window
      );
      assert(get_ret == MPI_SUCCESS and "MPI_Get: Should be successful");
    } else if (op_type == rdma_type_t::Put) {
      auto const& put_ret = MPI_Put(
        ptr, ptr_num_bytes, MPI_BYTE, target, d_offset, num_bytes, MPI_BYTE, window
      );
      assert(put_ret == MPI_SUCCESS and "MPI_Put: Should be successful");
    } else {
      assert(0 and "op_type must be Get or Put");
    }
  }

  void
  free_channel() {
    if (locked) {
      unlock_channel_for_op();
    }
    if (initialized) {
      MPI_Win_free(&window);
      MPI_Group_free(&channel_group);
      MPI_Comm_free(&channel_comm);
    }
  }

  virtual ~Channel() {
    free_channel();
  }

private:
  void
  init_channel_window() {
    debug_print_rdma_channel(
      "%d: channel: create window: num_bytes=%lld\n", my_node, num_bytes
    );

    int win_create_ret = 0;

    if (is_target) {
      win_create_ret = MPI_Win_create(
        ptr, num_bytes, rdma_elm_size, MPI_INFO_NULL, channel_comm, &window
      );
    } else {
      win_create_ret = MPI_Win_create(
        nullptr, 0, 1, MPI_INFO_NULL, channel_comm, &window
      );
    }

    assert(
      win_create_ret == MPI_SUCCESS and "MPI_Win_create: Should be successful"
    );

    initialized = true;

    debug_print_rdma_channel(
      "%d: channel: init_channel: finished creating window\n", my_node
    );
  }

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
