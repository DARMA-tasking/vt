
#if !defined INCLUDED_BITS_PACKER
#define INCLUDED_BITS_PACKER

#include <cassert>
#include <cstdlib>

namespace vt { namespace utils {

struct BitPacker {
  using FieldType         = int64_t;
  using FieldUnsignedType = uint64_t;

  #define gen_bit_mask(len) ((static_cast<uint64_t>(1) << (len)) - 1)

  template <typename BitField>
  static inline FieldUnsignedType getMsbBit(BitField const& field) {
    uint64_t field_copy = static_cast<uint64_t>(field);
    FieldUnsignedType r = 0;
    while (field_copy >>= 1) {
      r++;
    }
    return r;
  }

  template <FieldType start, FieldType len, typename BitSegment, typename BitField>
  static inline void bitorSetField(BitField& field, BitSegment const& segment) {
    field |= segment << start;
  }

  template <FieldType start, FieldType len, typename BitSegment, typename BitField>
  static inline BitSegment getField(BitField const& field) {
    return getFieldDynamic<BitSegment, BitField>(start, len, field);
  }

  template <typename BitSegment, typename BitField>
  static inline BitSegment getFieldDynamic(
    FieldType start, FieldType len, BitField const& field
  ) {
    auto const& comp = field >> start;
    ::fmt::print(
      "start={}, len={}, field={}, comp={}, sizeof(BitField)={}, masked={},"
      "mask={}\n",
      start, len, field, comp, sizeof(BitField), comp & gen_bit_mask(len),
      gen_bit_mask(len)
    );
    return len < sizeof(BitField)*8 ?
      static_cast<BitSegment>(comp & gen_bit_mask(len)) :
      static_cast<BitSegment>(comp);
  }

  template <FieldType start, FieldType len, typename BitSegment, typename BitField>
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
    FieldType start, FieldType len, BitField& field, BitSegment const& segment
  ) {
    field = field & ~(gen_bit_mask(len) << start);
    field = field | (static_cast<BitField>(segment) << start);
  }

  template <FieldType start, FieldType len = 1, typename BitField>
  static inline void boolSetField(BitField& field, bool const& set_value) {
    if (set_value) {
      field |= 1UL << start;
    } else {
      field &= ~(1UL << start);
    }
  }

  template <FieldType start, FieldType len = 1, typename BitField>
  static inline bool boolGetField(BitField const& field) {
    // return field & (1 << start);
    BitField field_copy = field;
    bool const is_set = field_copy & (1UL << start);
    return is_set;
  }

};

}}  // end namespace vt::utils

#endif  /*INCLUDED_BITS_PACKER*/
