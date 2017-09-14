
#if !defined INCLUDED_BITS_COMMON
#define INCLUDED_BITS_COMMON

#include "common.h"
#include "bits_counter.h"
#include "bits_packer.h"

namespace vt {

template<typename T>
using BitCounterType = utils::BitCounter<T>;
using BitPackerType = utils::BitPacker;

}  // end namespace vt

#endif  /*INCLUDED_BITS_COMMON*/
