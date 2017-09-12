
#if ! defined __RUNTIME_TRANSPORT_RDMA_PENDING__
#define __RUNTIME_TRANSPORT_RDMA_PENDING__

#include "common.h"
#include "function.h"
#include "rdma_common.h"

namespace runtime { namespace rdma {

struct Pending {
  rdma_recv_t cont = nullptr;
  ActionType cont2 = nullptr;
  rdma_ptr_t data_ptr = nullptr;

  Pending(rdma_recv_t in_cont)
    : cont(in_cont)
  { }

  Pending(ActionType in_cont2)
    : cont2(in_cont2)
  { }

  Pending(rdma_ptr_t in_data_ptr, ActionType in_cont2)
    : data_ptr(in_data_ptr), cont2(in_cont2)
  { }
};


}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_PENDING__*/
