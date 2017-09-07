
#if ! defined __RUNTIME_TRANSPORT_BIT_COMMON__
#define __RUNTIME_TRANSPORT_BIT_COMMON__

#include "common.h"
#include "bit_counter.h"
#include "bit_packer.h"

namespace runtime {

template <typename T>
using bit_counter_t = util::BitCounter<T>;
using bit_packer_t = util::BitPacker;

} // end namespace runtime

#endif /*__RUNTIME_TRANSPORT_BIT_COMMON__*/
