
#if !defined INCLUDED_VT_COLLECTIVE_COLLECTIVE_H
#define INCLUDED_VT_COLLECTIVE_COLLECTIVE_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/runtime/runtime_headers.h"
#include "vt/collective/collective_ops.h"

namespace vt {

// vt::{initialize,finalize} exported into the main ::vt namespace
inline RuntimePtrType initialize(
  int& argc, char**& argv, WorkerCountType const num_workers = no_workers,
  bool is_interop = false, MPI_Comm* comm = nullptr
) {
  return ::vt::CollectiveOps::initialize(argc,argv,num_workers,is_interop,comm);
}

inline RuntimePtrType initialize(
  int& argc, char**& argv, MPI_Comm* comm = nullptr
) {
  bool const is_interop = comm != nullptr;
  return ::vt::CollectiveOps::initialize(argc,argv,no_workers,is_interop,comm);
}

inline RuntimePtrType initialize(MPI_Comm* comm = nullptr) {
  int argc = 0;
  char** argv = nullptr;
  return ::vt::CollectiveOps::initialize(argc,argv,no_workers,true,comm);
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
