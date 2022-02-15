/*
//@HEADER
// *****************************************************************************
//
//                           test_memory_footprint.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include <checkpoint/checkpoint.h>
#include "vt/configs/arguments/args.h"
#include "vt/timetrigger/time_trigger_manager.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/stats_restart_reader.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/rdmahandle/manager.h"
#include "vt/collective/collective_alg.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/group/group_manager.h"
#include "vt/vrt/collection/manager.h"
#include "vt/topos/location/manager.h"
#include "vt/termination/termination.h"
#include "vt/sequence/sequencer_headers.h"
#include "vt/scheduler/scheduler.h"
#include "vt/rdma/rdma.h"
#include "vt/parameterization/parameterization.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/pool/pool.h"
#include "vt/context/context.h"
#include "vt/trace/trace.h"
#include "vt/pmpi/pmpi_component.h"

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

using TestMemoryFootprinting = TestParallelHarness;

TEST_F(TestMemoryFootprinting, test_live_components) {
  vt::rt->printMemoryFootprint();
}

TEST_F(TestMemoryFootprinting, test_live_components_at_shutdown) {
  theConfig()->vt_print_memory_footprint = true;
}

template<typename T>
void printMemoryFootprint(T* component) {
  if (component != nullptr) {
    fmt::print(
      "Memory footprint for component {}:\t{}\n",
      component->name(),
      checkpoint::getMemoryFootprint(*component)
    );
  }
}

TEST_F(TestMemoryFootprinting, test_time_trigger_manager) {
  printMemoryFootprint(theTimeTrigger());
}

TEST_F(TestMemoryFootprinting, test_lb_manager) {
  printMemoryFootprint(theLBManager());
}

TEST_F(TestMemoryFootprinting, test_stats_restart_reader) {
  vt::vrt::collection::balance::StatsRestartReader reader;
  printMemoryFootprint(&reader);
}

TEST_F(TestMemoryFootprinting, test_node_stats) {
  vt::vrt::collection::balance::NodeStats stats;
  printMemoryFootprint(theNodeStats());
}

TEST_F(TestMemoryFootprinting, test_memory_usage) {
  printMemoryFootprint(theMemUsage());
}

TEST_F(TestMemoryFootprinting, test_rdma_manager_handle) {
  printMemoryFootprint(theHandleRDMA());
}

TEST_F(TestMemoryFootprinting, test_collective_alg) {
  printMemoryFootprint(theCollective());
}

TEST_F(TestMemoryFootprinting, test_obj_group_manager) {
  printMemoryFootprint(theObjGroup());
}

TEST_F(TestMemoryFootprinting, test_pipe_manager) {
  printMemoryFootprint(theCB());
}

TEST_F(TestMemoryFootprinting, test_group_manager) {
  printMemoryFootprint(theGroup());
}

TEST_F(TestMemoryFootprinting, test_collection_manager) {
  printMemoryFootprint(theCollection());
}

TEST_F(TestMemoryFootprinting, test_location_manager) {
  printMemoryFootprint(theLocMan());
}

TEST_F(TestMemoryFootprinting, test_termination_detector) {
  printMemoryFootprint(theTerm());
}

TEST_F(TestMemoryFootprinting, test_sequencer_virtual) {
  printMemoryFootprint(theVirtualSeq());
}

TEST_F(TestMemoryFootprinting, test_sequencer) {
  printMemoryFootprint(theSeq());
}

TEST_F(TestMemoryFootprinting, test_scheduler) {
  printMemoryFootprint(theSched());
}

TEST_F(TestMemoryFootprinting, test_registry) {
  printMemoryFootprint(theRegistry());
}

TEST_F(TestMemoryFootprinting, test_rdma_manager) {
  printMemoryFootprint(theRDMA());
}

TEST_F(TestMemoryFootprinting, test_param) {
  printMemoryFootprint(theParam());
}

TEST_F(TestMemoryFootprinting, test_active_messenger) {
  printMemoryFootprint(theMsg());
}

TEST_F(TestMemoryFootprinting, test_async_event) {
  vt::event::AsyncEvent event;
  printMemoryFootprint(&event);
}

#if vt_threading_enabled
TEST_F(TestMemoryFootprinting, test_worker_group_type) {
  printMemoryFootprint(theWorkerGrp());
}
#endif

TEST_F(TestMemoryFootprinting, test_virtual_context_manager) {
  printMemoryFootprint(theVirtualManager());
}

TEST_F(TestMemoryFootprinting, test_pool) {
  printMemoryFootprint(thePool());
}

TEST_F(TestMemoryFootprinting, test_context) {
  printMemoryFootprint(theContext());
}

TEST_F(TestMemoryFootprinting, test_phase) {
  printMemoryFootprint(thePhase());
}

TEST_F(TestMemoryFootprinting, test_stats_reader) {
  printMemoryFootprint(theStatsReader());
}

TEST_F(TestMemoryFootprinting, test_epoch) {
  printMemoryFootprint(theEpoch());
}

#if vt_check_enabled(trace_enabled)
TEST_F(TestMemoryFootprinting, test_trace) {
  printMemoryFootprint(theTrace());
}
#endif

#if vt_check_enabled(mpi_access_guards)
TEST_F(TestMemoryFootprinting, test_pmpi_component) {
  printMemoryFootprint(thePMPI());
}
#endif

}}} /* end namespace vt::tests::unit */
