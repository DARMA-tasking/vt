
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_STRAT_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_STRAT_H

#include "vt/config.h"

#include <cstdlib>

namespace vt { namespace vrt { namespace collection { namespace lb {

enum struct HeapExtractEnum : int8_t {
  LoadOverLessThan    = 1,
  LoadOverGreaterThan = 2,
  LoadOverRandom      = 3,
  LoadOverOneEach     = 4
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_STRAT_H*/
