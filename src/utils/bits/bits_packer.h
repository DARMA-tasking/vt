
#if !defined INCLUDED_BITS_PACKER
#define INCLUDED_BITS_PACKER

#include <cassert>

namespace vt { namespace utils {

struct BitPacker {

  template <typename BitField>
  static inline uint8_t getMsbBit(BitField const& field) {
    uint64_t field_copy = static_cast<uint64_t>(field);
    uint8_t r = 0;
    while (field_copy >>= 1) {
      r++;
    }
    return r;
  }

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline void bitorSetField(BitField& field, BitSegment const& segment) {
    field |= segment << start;
  }

  #define gen_bit_mask(len) ((static_cast<uint64_t>(1) << (len)) - 1)

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline BitSegment getField(BitField const& field) {
    return getFieldDynamic<BitSegment, BitField>(start, len, field);
  }

  template <typename BitSegment, typename BitField>
  static inline BitSegment getFieldDynamic(
    int8_t start, int8_t len, BitField const& field
  ) {
    auto const& comp = field >> start;
    return
      len < sizeof(BitField) ?
      static_cast<BitSegment>(comp & gen_bit_mask(len)) :
      static_cast<BitSegment>(comp);
  }

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline void setField(BitField& field, BitSegment const& segment) {
    #if backend_check_enabled(bit_check_overflow)
    auto const& seg_msb_bit = get_msb_bit(segment);
    //fmt::print("size={}, high bit={}\n", sizeof(BitSegment)*8, seg_msb_bit);
    assert(
      seg_msb_bit <= len and
      "bit_check_overflow: value in segment overflows specified bit length"
    );
    #endif

    return BitPacker::setFieldDynamic<BitSegment, BitField>(
      start, len, field, segment
    );
  }

  template <typename BitSegment, typename BitField>
  static inline void setFieldDynamic(
    int8_t start, int8_t len, BitField& field, BitSegment const& segment
  ) {
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

}}  // end namespace vt::utils

#endif  /*INCLUDED_BITS_PACKER*/
