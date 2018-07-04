
#if !defined INCLUDED_UTILS_BITS_BITS_PACKER_IMPL_H
#define INCLUDED_UTILS_BITS_BITS_PACKER_IMPL_H

#inlcude "config.h"

#include <cstdlib>

namespace vt { namespace utils {

#define gen_bit_mask(len) ((static_cast<uint64_t>(1) << (len)) - 1)

template <typename BitField>
/*static*/ inline FieldUnsignedType
BitPacker::getMsbBit(BitField const& field) {
  uint64_t field_copy = static_cast<uint64_t>(field);
  FieldUnsignedType r = 0;
  while (field_copy >>= 1) {
    r++;
  }
  return r;
}

template <FieldType start, FieldType len, typename BitType, typename BitField>
/*static*/ inline void
BitPacker::bitorSetField(BitField& field, BitType const& segment) {
  field |= segment << start;
}

template <FieldType start, FieldType len, typename BitType, typename BitField>
/*static*/ inline BitType
BitPacker::getField(BitField const& field) {
  return getFieldDynamic<BitType, BitField>(start, len, field);
}

template <typename BitType, typename BitField>
/*static*/ inline BitType BitPacker::getFieldDynamic(
  FieldType start, FieldType len, BitField const& field
) {
  auto const& comp = field >> start;
  // ::fmt::print(
  //   "start={}, len={}, field={}, comp={}, sizeof(BitField)={}, masked={},"
  //   "mask={}\n",
  //   start, len, field, comp, sizeof(BitField), comp & gen_bit_mask(len),
  //   gen_bit_mask(len)
  // );
  return len < sizeof(BitField) * 8 ?
    static_cast<BitType>(comp & gen_bit_mask(len)) :
    static_cast<BitType>(comp);
}

template <FieldType start, FieldType len, typename BitType, typename BitField>
/*static*/ inline void
BitPacker::setField(BitField& field, BitType const& segment) {
  #if backend_check_enabled(bit_check_overflow)
    auto const& seg_msb_bit = get_msb_bit(segment);
    //fmt::print("size={}, high bit={}\n", sizeof(BitType)*8, seg_msb_bit);
    assert(
      seg_msb_bit <= len and
      "bit_check_overflow: value in segment overflows specified bit length"
    );
  #endif

  return BitPacker::setFieldDynamic<BitType, BitField>(
    start, len, field, segment
  );
}

template <typename BitType, typename BitField>
/*static*/ inline void
BitPacker::setFieldDynamic(
  FieldType start, FieldType len, BitField& field, BitType const& segment
) {
  field = field & ~(gen_bit_mask(len) << start);
  field = field | (static_cast<BitField>(segment) << start);
}

template <FieldType start, FieldType len, typename BitField>
/*static*/ inline void
BitPacker::boolSetField(BitField& field, bool const& set_value) {
  if (set_value) {
    field |= 1UL << start;
  } else {
    field &= ~(1UL << start);
  }
}

template <FieldType start, FieldType len, typename BitField>
/*static*/ inline bool
BitPacker::boolGetField(BitField const& field) {
  // return field & (1 << start);
  BitField field_copy = field;
  bool const is_set = field_copy & (1UL << start);
  return is_set;
}


}} /* end namespace vt::utils */

#endif /*INCLUDED_UTILS_BITS_BITS_PACKER_IMPL_H*/
