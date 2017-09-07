
#if ! defined __RUNTIME_TRANSPORT_BIT_PACKER__
#define __RUNTIME_TRANSPORT_BIT_PACKER__

namespace runtime { namespace util {

struct BitPacker {

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline void
  bitor_set_field(BitField& field, BitSegment const& segment) {
    field |= segment << start;
  }

  #define gen_bit_mask(len) ((static_cast<uint64_t>(1) << (len)) - 1)

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline BitSegment
  get_field(BitField const& field) {
    return static_cast<BitSegment>((field >> start) & gen_bit_mask(len));
  }

  template <int8_t start, int8_t len, typename BitSegment, typename BitField>
  static inline void
  set_field(BitField& field, BitSegment const& segment) {
    field = field & ~(gen_bit_mask(len) << start);
    field = field | (static_cast<BitField>(segment) << start);
  }

  template <int8_t start, int8_t len = 1, typename BitField>
  static inline void
  bool_set_field(BitField& field, bool const& set_value) {
    if (set_value) {
      field |= 1 << start;
    } else {
      field &= ~(1 << start);
    }
  }

  template <int8_t start, int8_t len = 1, typename BitField>
  static inline bool
  bool_get_field(BitField& field) {
    return field & (1 << start);
  }

};

}} //end namespace runtime::util

#endif /*__RUNTIME_TRANSPORT_BIT_PACKER__*/
