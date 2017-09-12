
#if ! defined __RUNTIME_TRANSPORT_RDMA_PENDING__
#define __RUNTIME_TRANSPORT_RDMA_PENDING__

#include "common.h"
#include "function.h"
#include "rdma_common.h"

namespace vt { namespace rdma {

struct Pending {
  RDMA_RecvType cont = nullptr;
  ActionType cont2 = nullptr;
  RDMA_PtrType data_ptr = nullptr;

  Pending(RDMA_RecvType in_cont)
    : cont(in_cont)
  { }

  Pending(ActionType in_cont2)
    : cont2(in_cont2)
  { }

  Pending(RDMA_PtrType in_data_ptr, ActionType in_cont2)
    : cont2(in_cont2), data_ptr(in_data_ptr)
  { }
};


}} //end namespace vt::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_PENDING__*/
