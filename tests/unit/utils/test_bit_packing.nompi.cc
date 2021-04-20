/*
//@HEADER
// *****************************************************************************
//
//                         test_bit_packing.nompi.cc
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

#include <gtest/gtest.h>

#include "vt/utils/bits/bits_packer.h"
#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

using TestBitPacking = TestHarness;

TEST_F(TestBitPacking, test_bit_packing_1) {
  using vt::utils::BitPacker;
  using Field = uint64_t;
  Field x = 0xFAADFEEDFBBBFCCDull;
  auto x0 = BitPacker::getField<0, 16, Field>(x);
  auto x1 = BitPacker::getField<16, 16, Field>(x);
  auto x2 = BitPacker::getField<32, 16, Field>(x);
  auto x3 = BitPacker::getField<48, 16, Field>(x);

  EXPECT_EQ(x0, 0xFCCDull);
  EXPECT_EQ(x1, 0xFBBBull);
  EXPECT_EQ(x2, 0xFEEDull);
  EXPECT_EQ(x3, 0xFAADull);
}

TEST_F(TestBitPacking, test_bit_packing_2) {
  using vt::utils::BitPacker;
  using Field = uint64_t;
  Field x = 0xFFFFFFFFFFFFFFFFull;
  auto x0 = 0xFCCDull;
  auto x1 = 0xFBBBull;
  auto x2 = 0xFEEDull;
  auto x3 = 0xFAADull;

  BitPacker::setField<0, 16, Field>(x, x0);
  BitPacker::setField<16, 16, Field>(x, x1);
  BitPacker::setField<32, 16, Field>(x, x2);
  BitPacker::setField<48, 16, Field>(x, x3);

  EXPECT_EQ(x, 0xFAADFEEDFBBBFCCDull);
}

TEST_F(TestBitPacking, test_bit_packing_dynamic_3) {
  using vt::utils::BitPacker;
  using Field = uint64_t;
  Field x = 0xFAADFEEDFBBBFCCDull;
  auto x0 = BitPacker::getFieldDynamic<Field>(0, 16, x);
  auto x1 = BitPacker::getFieldDynamic<Field>(16, 16, x);
  auto x2 = BitPacker::getFieldDynamic<Field>(32, 16, x);
  auto x3 = BitPacker::getFieldDynamic<Field>(48, 16, x);

  EXPECT_EQ(x0, 0xFCCDull);
  EXPECT_EQ(x1, 0xFBBBull);
  EXPECT_EQ(x2, 0xFEEDull);
  EXPECT_EQ(x3, 0xFAADull);
}

TEST_F(TestBitPacking, test_bit_packing_dynamic_4) {
  using vt::utils::BitPacker;
  using Field = uint64_t;
  Field x = 0xFFFFFFFFFFFFFFFFull;
  auto x0 = 0xFCCDull;
  auto x1 = 0xFBBBull;
  auto x2 = 0xFEEDull;
  auto x3 = 0xFAADull;

  BitPacker::setFieldDynamic<Field>(0, 16, x, x0);
  BitPacker::setFieldDynamic<Field>(16, 16, x, x1);
  BitPacker::setFieldDynamic<Field>(32, 16, x, x2);
  BitPacker::setFieldDynamic<Field>(48, 16, x, x3);

  EXPECT_EQ(x, 0xFAADFEEDFBBBFCCDull);
}

TEST_F(TestBitPacking, test_bit_packing_dynamic_check_masking_5) {
  using vt::utils::BitPacker;
  using Field = uint64_t;
  Field x = 0x000000000000FEEDull;
  auto x0 = 0xFFFFFFFFFFFFFCCDull;

  BitPacker::setFieldDynamic<Field>(32, 16, x, x0);

  EXPECT_EQ(x, 0x0000FCCD0000FEEDull);
}

}}} /* end namespace vt::tests::unit */
