
#include "config.h"
#include "epoch/epoch.h"
#include "epoch/epoch_manip.h"

namespace vt { namespace epoch {

/*static*/ EpochType EpochManip::cur_rooted_ = no_epoch;
/*static*/ EpochType EpochManip::cur_non_rooted_ = no_epoch;

}} /* end namespace vt::epoch */
