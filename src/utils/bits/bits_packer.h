
#if !defined INCLUDED_BITS_PACKER
#define INCLUDED_BITS_PACKER

#include <cassert>
#include <cstdlib>

namespace vt { namespace utils {

struct BitPacker {
  using FieldType         = int64_t;
  using FieldUnsignedType = uint64_t;

  template <FieldType start, FieldType len, typename BitType, typename BitField>
  static inline BitType getField(BitField const& field);

  template <FieldType start, FieldType len, typename BitType, typename BitField>
  static inline void setField(BitField& field, BitType const& segment);

  template <FieldType start, FieldType len = 1, typename BitField>
  static inline void boolSetField(BitField& field, bool const& set_value);

  template <FieldType start, FieldType len = 1, typename BitField>
  static inline bool boolGetField(BitField const& field);

public:
  template <typename BitType, typename BitField>
  static inline BitType getFieldDynamic(
    FieldType start, FieldType len, BitField const& field
  );

  template <typename BitType, typename BitField>
  static inline void setFieldDynamic(
    FieldType start, FieldType len, BitField& field, BitType const& segment
  );

private:
  template <typename BitField>
  static inline FieldUnsignedType getMsbBit(BitField const& field);

  template <FieldType start, FieldType len, typename BitType, typename BitField>
  static inline void bitorSetField(BitField& field, BitType const& segment);

};

}}  // end namespace vt::utils

#include "utils/bits/bits_packer.impl.h"

#endif  /*INCLUDED_BITS_PACKER*/
