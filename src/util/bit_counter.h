
#if ! defined __RUNTIME_TRANSPORT_BIT_COUNTER__
#define __RUNTIME_TRANSPORT_BIT_COUNTER__

namespace runtime { namespace util {

template <typename BitField>
struct BitCounter {
  static constexpr bit_count_t const value = sizeof(BitField) * 8;
};

}} //end namespace runtime::util

#endif /*__RUNTIME_TRANSPORT_BIT_COUNTER__*/
