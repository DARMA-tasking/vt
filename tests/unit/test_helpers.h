/*
//@HEADER
// *****************************************************************************
//
//                               test_helpers.h
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

#if !defined INCLUDED_TEST_HELPERS_H
#define INCLUDED_TEST_HELPERS_H

#include "vt/context/context.h"
#include <gtest/gtest.h>

namespace vt { namespace tests { namespace unit {

/**
 * Maximum number of ranks/nodes detected by CMake on this machine.
 * Defaults to number of processors detected on the host system.
 */
constexpr NodeType CMAKE_DETECTED_MAX_NUM_NODES = vt_detected_max_num_nodes;


extern std::unique_ptr<MPISingletonMultiTest> mpi_singleton;

/**
 * Check whether we're oversubscribing on the current execution.
 * This is using MPI because it can be used before vt initializes.
 */
inline bool isOversubscribed() {
  assert((mpi_singleton) && "mpi_singleton should be initialized!");
  return mpi_singleton->getNumRanks() > CMAKE_DETECTED_MAX_NUM_NODES;
}

/**
 * The following helper macros (these have to be macros, because GTEST_SKIP
 * won't work from nested call) are meant to ensure that the test will be
 * run on desired number of nodes.
 *
 * For example, some tests are written in a way that they require at least 2
 * nodes while the others might take way too long to complete when being run
 * on more nodes than number of processors on the host system.
 */

/**
 * Used for tests that shouldn't run on more than 'max_req_num_nodes' nodes
 */
#define SET_MAX_NUM_NODES_CONSTRAINT(max_req_num_nodes)                   \
{                                                                         \
  auto const num_nodes = theContext()->getNumNodes();                     \
  if (num_nodes > max_req_num_nodes) {                                    \
    GTEST_SKIP() << fmt::format(                                          \
      "Skipping the run on {} nodes. This test should run on at most {} " \
      "nodes!\n",                                                         \
      num_nodes, max_req_num_nodes                                        \
    );                                                                    \
  }                                                                       \
}                                                                         \


/**
 * Used for tests that shouldn't run on less than 'min_req_num_nodes' nodes
 */
#define SET_MIN_NUM_NODES_CONSTRAINT(min_req_num_nodes)                   \
{                                                                         \
  auto const num_nodes = theContext()->getNumNodes();                     \
  if (num_nodes < min_req_num_nodes) {                                    \
    GTEST_SKIP() << fmt::format(                                          \
      "Skipping the run on {} nodes. This test should run on at least {} "\
      "nodes!\n",                                                         \
      num_nodes, min_req_num_nodes                                        \
    );                                                                    \
  }                                                                       \
}                                                                         \


/**
 * Used for tests that should only run on 'req_num_nodes'
 */
#define SET_NUM_NODES_CONSTRAINT(req_num_nodes)                           \
{                                                                         \
 auto const num_nodes = theContext()->getNumNodes();                      \
  if (num_nodes != req_num_nodes) {                                       \
    GTEST_SKIP() << fmt::format(                                          \
      "Skipping the run on {} nodes. This test should run only on {} "    \
      "nodes!\n",                                                         \
      num_nodes, req_num_nodes                                            \
    );                                                                    \
  }                                                                       \
}                                                                         \


/**
 * Used for tests that should only run on <'min_req_num_nodes', 'max_req_num_nodes'>
 * range of nodes
 */
#define SET_NUM_NODES_RANGE_CONSTRAINT(            \
min_req_num_nodes, max_req_num_nodes)              \
{                                                  \
  SET_MIN_NUM_NODES_CONSTRAINT(min_req_num_nodes); \
  SET_MAX_NUM_NODES_CONSTRAINT(max_req_num_nodes); \
}                                                  \



}}} // namespace vt::tests::unit

#endif /* INCLUDED_TEST_HELPERS_H */
