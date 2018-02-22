
#include "config.h"
#include "timing/timing.h"
#include "timing/timing_type.h"

#include <mpi.h>

namespace vt { namespace timing {

/*static*/ TimeType Timing::getCurrentTime() {
  return MPI_Wtime();
}

}} /* end namespace vt::timing */
