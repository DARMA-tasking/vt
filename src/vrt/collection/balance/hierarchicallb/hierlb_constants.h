
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H

#include "config.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

static constexpr double const hierlb_threshold = 0.5f;
static constexpr NodeType const hierlb_nary = 2;
static constexpr NodeType const hierlb_root = 0;
static constexpr int32_t const hierlb_bin_size = 10;
static constexpr double const hierlb_no_load_sentinel = -1.0f;
static constexpr double const hierlb_tolerance = 3.0f;
static constexpr bool const hierlb_auto_threshold = true;

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H*/
