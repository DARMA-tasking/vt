
#include "util.h"

#include <cstdint>

namespace fcontext { namespace examples {

void sleep(uint32_t const ms) {
  timespec req = { (time_t)ms / 1000, (long)((ms % 1000) * 1000000) };
  timespec rem = { 0, 0 };
  nanosleep(&req, &rem);
}

}} /* end namespace fcontext::examples */
