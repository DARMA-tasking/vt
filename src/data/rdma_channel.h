
#if ! defined __RUNTIME_TRANSPORT_RDMA_CHANNEL__
#define __RUNTIME_TRANSPORT_RDMA_CHANNEL__

#include "common.h"
#include "function.h"
#include "rdma_common.h"
#include "rdma_state.h"
#include "rdma_handle.h"
#include "rdma_msg.h"

#include <mpi.h>

#include <unordered_map>

namespace runtime { namespace rdma {

static constexpr byte_t const rdma_elm_size = sizeof(char);
static constexpr byte_t const rdma_empty_byte = 0;

struct Channel {
  using rdma_handle_manager_t = HandleManager;

  Channel(
    rdma_handle_t const& in_rdma_handle, bool const& in_is_target,
    rdma_ptr_t const& in_ptr = nullptr, byte_t const& in_num_bytes = no_byte
  ) : rdma_handle(in_rdma_handle), is_target(in_is_target),
      ptr(in_ptr), num_bytes(in_num_bytes)
  {
    dest_node = rdma_handle_manager_t::get_rdma_node(rdma_handle);
    my_node = the_context->get_node();
    assert(
      dest_node != my_node and
      "A RDMA Channel can not be created within the same node"
    );
    if (in_num_bytes == no_byte) {
      num_bytes = rdma_empty_byte;
    }
  }

  void
  initialize_channel() {
    if (is_target) {
      MPI_Win_create(
        ptr, num_bytes, rdma_elm_size, MPI_INFO_NULL, MPI_COMM_WORLD, &window
      );
    } else {
      MPI_Win_create(
        nullptr, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window
      );
    }
  }

private:
  bool const is_target;

  bool initialized = false;
  MPI_Win window;
  rdma_handle_t const rdma_handle = no_rdma_handle;
  node_t dest_node = uninitialized_destination;
  node_t my_node = uninitialized_destination;
  byte_t num_bytes = no_byte;
  rdma_ptr_t ptr = no_rdma_ptr;
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
