/*
//@HEADER
// ************************************************************************
//
//                          index_test.cc
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

#include <cassert>

#include "vt/transport.h"

using namespace vt;

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& num_nodes = theContext()->getNumNodes();

  Index2D idx(2, 3);
  Index2D idx2(5, 10);
  Index2D idx3 = idx2 - idx;

  Index1D idx_1d(10);
  Index1D idx_1d_2(20);

  auto node = mapping::dense1DBlockMap(&idx_1d, &idx_1d_2, num_nodes);

  fmt::print(
    "idx={}, idx2={}, idx3={}, size={}, node={}\n",
    idx.toString().c_str(), idx2.toString().c_str(), idx3.toString().c_str(),
    sizeof(idx), node
  );

  int const dim1 = 4, dim2 = 5;
  Index2D idx_a(2, 2);
  Index2D idx_a_max(dim1, dim2);

  auto node_a = mapping::dense2DBlockMap(&idx_a, &idx_a_max, num_nodes);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    for (int j = 0; j < dim2; j++) {
      auto cur_idx = Index2D(i,j);
      auto lin_idx = mapping::linearizeDenseIndexColMajor(&cur_idx, &idx_a_max);
      fmt::print(
        "idx={}, max={}, lin={}\n",
        cur_idx.toString().c_str(), idx_a_max.toString().c_str(), lin_idx
      );
      vtAssertExpr(lin_idx == cur_val);
      cur_val++;
    }
  }

  auto const& idx_a_str = idx_a.toString().c_str();
  auto const& idx_a_max_str = idx_a_max.toString().c_str();

  fmt::print(
    "idx_a={}, indx_a_max={}, node={}\n", idx_a_str, idx_a_max_str, node_a
  );

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
