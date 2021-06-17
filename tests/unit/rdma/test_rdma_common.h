/*
//@HEADER
// *****************************************************************************
//
//                            test_rdma_common.cc
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

#if !defined INCLUDED_VT_TESTS_RDMA_COMMON_H
#define INCLUDED_VT_TESTS_RDMA_COMMON_H

#include <gtest/gtest.h>
#include "vt/configs/types/types_type.h"

namespace vt { namespace tests { namespace unit {

template <typename T>
struct UpdateData {
  template <typename HandleT>
  static void init(
    HandleT& handle, int space, std::size_t size, vt::NodeType rank
) {
    handle.modifyExclusive([=](T* val, std::size_t count){
      setMem(val, space, size, rank, 0);
    });
  }

  static void setMem(
    T* ptr, int space, std::size_t size, vt::NodeType rank, std::size_t offset
  ) {
    for (std::size_t i = offset; i < size; i++) {
      ptr[i] = static_cast<T>(space * rank + i);
    }
  }

  static void test(
    std::unique_ptr<T[]> ptr, int space, std::size_t size, vt::NodeType rank,
    std::size_t offset, T val = T{}
  ) {
    for (std::size_t i = offset; i < size; i++) {
      EXPECT_EQ(ptr[i], static_cast<T>(space * rank + i + val));
    }
  }
};

}}} /* end namespace vt::tests::unit */

#endif /*INCLUDED_VT_TESTS_RDMA_COMMON_H*/
