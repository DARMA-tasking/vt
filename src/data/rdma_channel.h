
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

    printf(
      "%d: channel: construct: target=%d, non_target=%d, my_node=%d, han=%lld, "
      "ptr=%p, bytes=%lld, is_target=%s\n",
      my_node, target, non_target, my_node, rdma_handle, ptr, num_bytes,
      print_bool(is_target)
    );
  }

  void
  init_channel_group() {
    printf(
      "%d: channel: init_channel_group: target=%d, my_node=%d, han=%lld\n",
      my_node, target, my_node, rdma_handle
    );

    MPI_Group world;
    MPI_Comm_group(MPI_COMM_WORLD, &world);

    int const channel_group_nodes[2] = {target, non_target};
    MPI_Group_incl(world, 2, channel_group_nodes, &channel_group);

    MPI_Comm_create_group(
      MPI_COMM_WORLD, channel_group, channel_group_tag, &channel_comm
    );

    printf("%d: channel: init_channel_group: finished\n", my_node);

    init_channel_window();
  }

  void
  write_data_to_channel(rdma_ptr_t const& ptr, byte_t const& ptr_num_bytes) {
    assert(initialized and "Channel must be initialized");
    assert(not target and "The target can not write to this channel");

    constexpr int const mpi_win_lock_assert_arg = 0;
    constexpr byte_t const mpi_target_disp = 0;

    auto const& lock_type =
      op_type == rdma_type_t::Get ? MPI_LOCK_SHARED : MPI_LOCK_EXCLUSIVE;

    printf(
      "%d: write_data_to_channel: target=%d, ptr=%p, ptr_num_bytes=%lld, "
      "num_bytes=%lld, op_type=%s, lock_type=%d\n",
      my_node, target, ptr, ptr_num_bytes, num_bytes,
      op_type == rdma_type_t::Get ? "GET" : "PUT",  lock_type
    );

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, mpi_win_lock_assert_arg, window);

    if (op_type == rdma_type_t::Get) {
      MPI_Get(
        ptr, ptr_num_bytes, MPI_BYTE, target, mpi_target_disp,
        num_bytes, MPI_BYTE, window
      );
    } else if (op_type == rdma_type_t::Put) {
      MPI_Put(
        ptr, ptr_num_bytes, MPI_BYTE, target, mpi_target_disp,
        num_bytes, MPI_BYTE, window
      );
    } else {
      assert(0 and "op_type must be Get or Put");
    }

    MPI_Win_unlock(target, window);
  }

  void
  free_channel() {
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
    printf("%d: channel: create window: num_bytes=%lld\n", my_node, num_bytes);

    if (is_target) {
      MPI_Win_create(
        ptr, num_bytes, rdma_elm_size, MPI_INFO_NULL, channel_comm, &window
      );
    } else {
      MPI_Win_create(
        nullptr, 0, 1, MPI_INFO_NULL, channel_comm, &window
      );
    }
    initialized = true;

    printf("%d: channel: init_channel: finished creating window\n", my_node);
  }

private:
  bool const is_target;

  bool initialized = false;
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

 // if (rank == 0) {
 //        /* Rank 0 will be the caller, so null window */
 //        MPI_Win_create(NULL,0,1,
 //            MPI_INFO_NULL,MPI_COMM_WORLD,&win);
 //        /* Request lock of process 1 */
 //        MPI_Win_lock(MPI_LOCK_SHARED,1,0,win);
 //        MPI_Put(buf,1,MPI_INT,1,0,1,MPI_INT,win);
 //        /* Block until put succeeds */
 //        MPI_Win_unlock(1,win);
 //        /* Free the window */
 //        MPI_Win_free(&win);
 //    }
 //    else {
 //        /* Rank 1 is the target process */
 //        MPI_Win_create(buf,2*sizeof(int),sizeof(int),
 //            MPI_INFO_NULL, MPI_COMM_WORLD, &win);
 //        /* No sync calls on the target process! */
 //        MPI_Win_free(&win);
 //    }

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_CHANNEL__*/
