
#if ! defined __RUNTIME_TRANSPORT_BIT_PACKER__
#define __RUNTIME_TRANSPORT_BIT_PACKER__

#include <cassert>

namespace vt { namespace util {

struct BitPacker {

  template <typename BitField>
  static inline uint8_t getMsbBit(BitField const& field) {
    uint64_t field_copy = static_cast<uint64_t>(field);
    uint8_t r = 0;
    while (field_copy >>= 1) { r++; }
    return r;
  }

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline void bitorSetField(BitField& field, BitSegment const& segment) {
    field |= segment << start;
  }

  #define gen_bit_mask(len) ((static_cast<uint64_t>(1) << (len)) - 1)

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline BitSegment getField(BitField const& field) {
    return static_cast<BitSegment>((field >> start) & gen_bit_mask(len));
  }

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline void setField(BitField& field, BitSegment const& segment) {
    #if backend_check_enabled(bit_check_overflow)
    auto const& seg_msb_bit = get_msb_bit(segment);
    //printf("size=%d, high bit=%d\n", sizeof(BitSegment)*8, seg_msb_bit);
    assert(
      seg_msb_bit <= len and
      "bit_check_overflow: value in segment overflows specified bit length"
    );
    #endif

    field = field & ~(gen_bit_mask(len) << start);
    field = field | (static_cast<BitField>(segment) << start);
  }

  template <int8_t start, int8_t len = 1, typename BitField>
  static inline void boolSetField(BitField& field, bool const& set_value) {
    if (set_value) {
      field |= 1 << start;
    } else {
      field &= ~(1 << start);
    }
  }

  template <int8_t start, int8_t len = 1, typename BitField>
  static inline bool boolGetField(BitField& field) {
    return field & (1 << start);
  }

};

}} //end namespace vt::util

#endif /*__RUNTIME_TRANSPORT_BIT_PACKER__*/
