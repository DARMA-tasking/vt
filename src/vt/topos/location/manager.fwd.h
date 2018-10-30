
#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_FWD_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_FWD_H

#include "config.h"

namespace vt { namespace location {

struct LocationManager;

}} /* end namespace vt::location */

namespace vt {

extern location::LocationManager* theLocMan();

}  // end namespace vt

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_FWD_H*/
