/*
//@HEADER
// *****************************************************************************
//
//                                vt_gtest.h
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

///
/// A dress-up/wrapper for GoogleTest and 1.8 + 1.10 compatibility.
/// Provides a shim during transition and to hide some slight awkwardness
/// with macros and changes between versions.
///
/// Use
///     #include "vt_gtest.h"
/// instead of
///     #include <gtest/gtest.h>
///
/// This is secondary from election to use a standard VT harness.
///

#ifndef __VIRTUAL_TRANSPORT_VT_GTEST__
#define __VIRTUAL_TRANSPORT_VT_GTEST__

// This define detects some future-breaking API without updating gtest target.
// (The resulting tests are ill-defined against an incompatile gtest lib.)
//#define GTEST_REMOVE_LEGACY_TEST_CASEAPI_ 1

#include <gtest/gtest.h>

// 1.8 -> 1.10 changes "Test Case" to "Test Suite" and deprecates
// the former; these are forward-compatible shims for gtest 1.8 builds.
// Larger changes to method names and such can't be done pre-1.10.

#ifndef TYPED_TEST_SUITE_P // gtest < 1.10 (should be 1.8)

#define TYPED_TEST_SUITE_P TYPED_TEST_CASE_P
#define REGISTER_TYPED_TEST_SUITE_P REGISTER_TYPED_TEST_CASE_P
#define INSTANTIATE_TYPED_TEST_SUITE_P INSTANTIATE_TYPED_TEST_CASE_P
#define INSTANTIATE_TEST_SUITE_P INSTANTIATE_TEST_CASE_P

// Unified macros to avoid having to specify a trailing comma to avoid
// pedantic warning about variadic arguments relating to name generation.
// Use the originals if wishing for name generators.
#define INSTANTIATE_TYPED_TEST_SUITE_P_VT(Prefix, Suite, Types) \
  INSTANTIATE_TYPED_TEST_CASE_P(Prefix, Suite, Types, /*...*/)
// 'test case' allowed name generatiom
#define INSTANTIATE_TEST_SUITE_P_VT(Prefix, Suite, Generator) \
  INSTANTIATE_TEST_CASE_P(Prefix, Suite, Generator, /*...*/)

#else

// Unified macros to avoid having to specify a trailing comma to avoid
// pedantic warning about variadic arguments relating to name generation.
// Use the originals if wishing for name generators.
#define INSTANTIATE_TYPED_TEST_SUITE_P_VT(Prefix, Suite, Types) \
  INSTANTIATE_TYPED_TEST_SUITE_P(Prefix, Suite, Types, /*...*/)
// 'test suite' does not allow name generation
#define INSTANTIATE_TEST_SUITE_P_VT(Prefix, Suite, Generator) \
  INSTANTIATE_TEST_SUITE_P(Prefix, Suite, Generator)

#endif


#endif /* __VIRTUAL_TRANSPORT_VT_GTEST__ */
