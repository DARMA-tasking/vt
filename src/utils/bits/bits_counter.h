
#if !defined INCLUDED_BITS_COUNTER
#define INCLUDED_BITS_COUNTER

#include "configs/types/types_common.h"

namespace vt {
namespace utils {

template<typename BitField>
struct BitCounter {
  static constexpr BitCountType const value = sizeof(BitField) * 8;
};

}  // end namespace utils
}  // end namespace vt

#endif  /*INCLUDED_BITS_COUNTER*/
