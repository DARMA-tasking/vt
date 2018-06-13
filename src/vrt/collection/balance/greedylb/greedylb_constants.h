
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CONSTANTS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CONSTANTS_H

#include "config.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

static constexpr NodeType const greedy_root = 0;
static constexpr int32_t const greedy_bin_size = 10;
static constexpr bool const greedy_auto_threshold = true;
static constexpr double const greedy_tolerance = 5.0f;
static constexpr double const greedy_threshold = 0.3f;
static constexpr double const greedy_max_threshold = 1.004f;

#define greedylb_use_parserdes 0

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CONSTANTS_H*/
