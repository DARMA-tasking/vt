/*
//@HEADER
// *****************************************************************************
//
//                              bits_packer.impl.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_UTILS_BITS_BITS_PACKER_IMPL_H
#define INCLUDED_UTILS_BITS_BITS_PACKER_IMPL_H

#include "vt/config.h"

#include <cstdlib>

namespace vt { namespace utils {

#define gen_bit_mask(len) ((static_cast<uint64_t>(1) << (len)) - 1)

using FieldType = BitPacker::FieldType;

template <typename BitField>
/*static*/ inline BitPacker::FieldUnsignedType
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
  return static_cast<size_t>(len) < sizeof(BitField) * 8 ?
    static_cast<BitType>(comp & gen_bit_mask(len)) :
    static_cast<BitType>(comp);
}

template <FieldType start, FieldType len, typename BitType, typename BitField>
/*static*/ inline void
BitPacker::setField(BitField& field, BitType const& segment) {
  #if vt_check_enabled(bit_check_overflow)
    auto const& seg_msb_bit = get_msb_bit(segment);
    //fmt::print("size={}, high bit={}\n", sizeof(BitType)*8, seg_msb_bit);
    vtAssert(
      seg_msb_bit <= len,
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
  auto const nbits = (sizeof(BitField) * 8) - len;
  field = field & ~(gen_bit_mask(len) << start);
  field = field | (((static_cast<BitField>(segment) << nbits) >> nbits) << start);
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
