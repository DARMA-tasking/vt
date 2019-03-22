/*
//@HEADER
// ************************************************************************
//
//                          test_collectives_scatter.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#define DEBUG_SCATTER 0

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

static constexpr std::size_t const num_elms = 4;

struct TestScatter : TestParallelHarness {
  static void scatterHan(int* msg) {
    auto const& this_node = theContext()->getNode();
    int* int_ptr = reinterpret_cast<int*>(msg);
    #if DEBUG_SCATTER
      ::fmt::print(
        "ptr={}, *ptr={}\n", print_ptr(int_ptr), *int_ptr
      );
    #endif
    for (auto i = 0; i < num_elms; i++) {
      #if DEBUG_SCATTER
        ::fmt::print(
          "i={}: this_node={}: val={}, expected={}\n",
          i, this_node, int_ptr[i], this_node * 10 + i
        );
      #endif
      EXPECT_EQ(int_ptr[i], this_node * 10 + i);
    }
  }
};

TEST_F(TestScatter, test_scatter_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& root = 0;

  if (this_node == 0) {
    auto const& elm_size = sizeof(int) * num_elms;
    auto const& total_size = elm_size * num_nodes;
    theCollective()->scatter<int,scatterHan>(
      total_size,elm_size,nullptr,[](NodeType node, void* ptr){
        auto ptr_out = reinterpret_cast<int*>(ptr);
        for (auto i = 0; i < num_elms; i++) {
          *(ptr_out + i) = node * 10 + i;
        }
      }
    );
  }
}


}}} // end namespace vt::tests::unit
