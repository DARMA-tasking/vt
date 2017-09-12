
#if ! defined __RUNTIME_TRANSPORT_BIT_COUNTER__
#define __RUNTIME_TRANSPORT_BIT_COUNTER__

namespace vt { namespace util {

template <typename BitField>
struct BitCounter {
  static constexpr BitCountType const value = sizeof(BitField) * 8;
};

}} //end namespace vt::util

#endif /*__RUNTIME_TRANSPORT_BIT_COUNTER__*/
