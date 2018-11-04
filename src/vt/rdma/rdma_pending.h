
#if !defined INCLUDED_RDMA_RDMA_PENDING_H
#define INCLUDED_RDMA_RDMA_PENDING_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/rdma/rdma_common.h"

namespace vt { namespace rdma {

struct Pending {
  RDMA_RecvType cont = nullptr;
  ActionType cont2 = nullptr;
  RDMA_PtrType data_ptr = nullptr;

  explicit Pending(RDMA_RecvType in_cont)
    : cont(in_cont)
  { }

  explicit Pending(ActionType in_cont2)
    : cont2(in_cont2)
  { }

  Pending(RDMA_PtrType in_data_ptr, ActionType in_cont2)
    : cont2(in_cont2), data_ptr(in_data_ptr)
  { }
};


}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_PENDING_H*/
