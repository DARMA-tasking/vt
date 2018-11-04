
#include "vt/config.h"
#include "vt/epoch/epoch.h"
#include "vt/epoch/epoch_manip.h"

namespace vt { namespace epoch {

/*static*/ EpochType EpochManip::cur_rooted_ = no_epoch;
/*static*/ EpochType EpochManip::cur_non_rooted_ = no_epoch;

}} /* end namespace vt::epoch */
