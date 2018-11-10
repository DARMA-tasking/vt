
#if !defined INCLUDED_VT_COLLECTIVE_COLLECTIVE_H
#define INCLUDED_VT_COLLECTIVE_COLLECTIVE_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/runtime/runtime_headers.h"
#include "vt/collective/collective_ops.h"

namespace vt {

inline RuntimePtrType initialize(int ac, char** av, MPI_Comm* comm = nullptr) {
  bool const is_interop = comm != nullptr;
  return ::vt::CollectiveOps::initialize(ac,av,no_workers,is_interop,comm);
}

inline RuntimePtrType initialize(MPI_Comm* comm = nullptr) {
  return ::vt::initialize(0,nullptr,comm);
}

inline void finalize(RuntimePtrType in_rt = nullptr) {
  if (in_rt) {
    return ::vt::CollectiveOps::finalize(std::move(in_rt));
  } else {
    return ::vt::CollectiveOps::finalize(nullptr);
  }
}

} /* end namespace vt */

#endif /*INCLUDED_VT_COLLECTIVE_COLLECTIVE_H*/
