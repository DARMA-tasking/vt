
#if !defined INCLUDED_TOPOS_LOCATION_RECORD_STATE_H
#define INCLUDED_TOPOS_LOCATION_RECORD_STATE_H

#include "config.h"
#include "context/context.h"

#include <cstdlib>
#include <cstdint>

namespace vt { namespace location {

enum class eLocState : int32_t {
  Local = 1,
  Remote = 2,
  Invalid = -1
};

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_RECORD_STATE_H*/
