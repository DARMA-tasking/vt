
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H

#include "config.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

static constexpr double const hierlb_threshold = 0.95f;
static constexpr NodeType const hierlb_nary = 3;
static constexpr NodeType const hierlb_root = 0;
static constexpr int32_t const hierlb_bin_size = 10;

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H*/
