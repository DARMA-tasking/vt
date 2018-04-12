
#include "config.h"
#include "omp_mutex.h"

#if backend_check_enabled(openmp)

#include <omp.h>

namespace vt { namespace util { namespace mutex {

OMPMutex::OMPMutex() {
  //fmt::print("constructing lock: ptr={}\n", &omp_lock);
  omp_init_lock(&omp_lock);
}

OMPMutex::~OMPMutex() {
  // There is a bug in OpenMP that causes this to segfault when it's called
  // after the OpenMP runtime is already finalized.

  //omp_destroy_lock(&omp_lock);
}

void OMPMutex::lock() {
  omp_set_lock(&omp_lock);
}

void OMPMutex::unlock() {
  omp_unset_lock(&omp_lock);
}

bool OMPMutex::try_lock() {
  int const ret = omp_test_lock(&omp_lock);
  return ret == 1;
}

}}} /* end namespace vt::util::mutex */

#endif
