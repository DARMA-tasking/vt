
#if ! defined __RUNTIME_TRANSPORT_BIT_COMMON__
#define __RUNTIME_TRANSPORT_BIT_COMMON__

#include "common.h"
#include "bit_counter.h"
#include "bit_packer.h"

namespace vt {

template <typename T>
using BitCounterType = util::BitCounter<T>;
using BitPackerType = util::BitPacker;

} // end namespace vt

#endif /*__RUNTIME_TRANSPORT_BIT_COMMON__*/
