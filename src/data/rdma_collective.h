
#if ! defined __RUNTIME_TRANSPORT_RDMA_COLLECTIVE__
#define __RUNTIME_TRANSPORT_RDMA_COLLECTIVE__

#include "common.h"
#include "function.h"
#include "rdma_common.h"
#include "rdma_state.h"
#include "rdma_handle.h"
#include "rdma_msg.h"

#include <mpi.h>

#include <unordered_map>

namespace runtime { namespace rdma {

struct Collective {
  using rdma_handle_manager_t = HandleManager;
};

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_COLLECTIVE__*/
