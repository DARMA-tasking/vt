
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CONSTANTS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CONSTANTS_H

#include "vt/config.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

static constexpr NodeType const greedy_root = 0;
static constexpr int32_t const greedy_bin_size = 10;
static constexpr bool const greedy_auto_threshold_p = true;
static constexpr double const greedy_tolerance = 5.0f;
static constexpr double const greedy_threshold_p = 0.3f;
static constexpr double const greedy_max_threshold_p = 1.004f;

#if backend_check_enabled(parserdes)
  // This is not a mistake, currently default off, even if parserdes is on
  #define greedylb_use_parserdes 0
#else
  #define greedylb_use_parserdes 0
#endif

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CONSTANTS_H*/
