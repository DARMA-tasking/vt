
#if !defined INCLUDED_TOPOS_LOCATION_UTILITY_COORD_H
#define INCLUDED_TOPOS_LOCATION_UTILITY_COORD_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"

namespace vt { namespace location {

// General base class for the location coords to erase templated types
struct LocationCoord {
  int data;
};

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_UTILITY_COORD_H*/
