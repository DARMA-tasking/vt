
#include "vt/config.h"
#include "vt/timing/timing.h"
#include "vt/timing/timing_type.h"

#include <mpi.h>

namespace vt { namespace timing {

/*static*/ TimeType Timing::getCurrentTime() {
  return MPI_Wtime();
}

}} /* end namespace vt::timing */
