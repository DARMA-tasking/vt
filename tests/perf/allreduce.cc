/*
//@HEADER
// *****************************************************************************
//
//                                 allreduce.cc
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

#include "common/test_harness.h"
#include "vt/collective/collective_alg.h"
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/collective/reduce/operators/functors/max_op.h"
#include "vt/collective/reduce/operators/functors/plus_op.h"
#include "vt/configs/error/config_assert.h"
#include "vt/configs/types/types_type.h"
#include "vt/context/context.h"
#include "vt/group/group_manager.h"
#include "vt/scheduler/scheduler.h"
#include <unordered_map>
#include <vt/collective/collective_ops.h>
#include <vt/objgroup/manager.h>
#include <vt/messaging/active.h>
#include <vt/collective/reduce/allreduce/rabenseifner.h>
#include <vt/collective/reduce/allreduce/recursive_doubling.h>
#include <vt/vrt/collection/manager.h>
#include <vt/collective/reduce/allreduce/state_holder.h>

#ifdef MAGISTRATE_KOKKOS_ENABLED
#include <Kokkos_Core.hpp>
#endif

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr std::array<size_t, 2> const payloadSizes = {
  2048};

struct MyTest : PerfTestHarness {
  MyTest() {
    DisableGlobalTimer();
    data.reserve(*std::max_element(payloadSizes.begin(), payloadSizes.end()));
  }

  std::vector<int32_t> data;
};

template <typename TestT>
struct NodeObj {
  explicit NodeObj(TestT* test_obj, const std::string& name)
    : base_name_(name),
      test_obj_(test_obj) {
    for (auto const payload_size : payloadSizes) {
      timer_names_[payload_size] =
        fmt::format("{} {}", base_name_, payload_size);
    }
  }

  void initialize() { proxy_ = vt::theObjGroup()->getProxy<NodeObj>(this); }

  void handlerVec(const std::vector<int32_t>& vec) {
    test_obj_->StopTimer(timer_names_.at(vec.size()));
    allreduce_done_ = true;
  }

#if MAGISTRATE_KOKKOS_ENABLED
  template <typename Scalar>
  void handlerView(Kokkos::View<Scalar*, Kokkos::HostSpace> view) {
    test_obj_->StopTimer(timer_names_.at(view.extent(0)));
    allreduce_done_ = true;
  }
#endif // MAGISTRATE_KOKKOS_ENABLED

  std::string base_name_ = {};
  std::unordered_map<size_t, std::string> timer_names_ = {};
  TestT* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
  bool allreduce_done_ = false;
};

// VT_PERF_TEST(MyTest, test_mpi_allreduce) {
//     for (auto payload_size : payloadSizes) {
//         data.resize(payload_size, theContext()->getNode() + 1);
//         std::vector<int> result(payload_size);
//         MPI_Barrier(MPI_COMM_WORLD);
//         StartTimer("MPI_Allreduce vector " + std::to_string(payload_size));
//         MPI_Allreduce(data.data(), result.data(), payload_size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
//         StopTimer("MPI_Allreduce vector " + std::to_string(payload_size));
//     }
// }

// VT_PERF_TEST(MyTest, test_reduce) {
//   auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj<MyTest>>(
//     "test_allreduce", this, "Reduce -> Bcast vector");

//   for (auto payload_size : payloadSizes) {
//     data.resize(payload_size, theContext()->getNode() + 1);

//     theCollective()->barrier();
//     auto* obj_ptr = grp_proxy[my_node_].get();
//     StartTimer(obj_ptr->timer_names_.at(payload_size));
//     grp_proxy.allreduce<&NodeObj<MyTest>::handlerVec, collective::PlusOp>(data);

//     theSched()->runSchedulerWhile(
//       [obj_ptr] { return !obj_ptr->allreduce_done_; });
//     obj_ptr->allreduce_done_ = false;
//   }
// }

#if MAGISTRATE_KOKKOS_ENABLED

struct MyTestKokkos : PerfTestHarness {
  MyTestKokkos() { DisableGlobalTimer(); }

  Kokkos::View<float*, Kokkos::HostSpace> view;
};

// VT_PERF_TEST(MyTestKokkos, test_mpi_allreduce_kokkos) {
//     for (auto payload_size : payloadSizes) {
//         view = Kokkos::View<float*, Kokkos::HostSpace>("view", payload_size);
//         auto result = Kokkos::View<float*, Kokkos::HostSpace>("result", payload_size);
//         MPI_Barrier(MPI_COMM_WORLD);
//         StartTimer("MPI_Allreduce view " + std::to_string(payload_size));
//         MPI_Allreduce(view.data(), result.data(), payload_size, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
//         StopTimer("MPI_Allreduce view " + std::to_string(payload_size));
//     }
// }

VT_PERF_TEST(MyTestKokkos, test_reduce_kokkos) {
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj<MyTestKokkos>>(
    "test_allreduce", this, "Reduce -> Bcast view");

  for (auto payload_size : payloadSizes) {
    view = Kokkos::View<float*, Kokkos::HostSpace>("view", payload_size);
    for (uint32_t i = 0; i < view.extent(0); ++i) {
      view(i) = static_cast<float>(my_node_);
    }

    theCollective()->barrier();
    auto* obj_ptr = grp_proxy[my_node_].get();
    StartTimer(obj_ptr->timer_names_.at(payload_size));

    grp_proxy.allreduce<
      &NodeObj<MyTestKokkos>::handlerView<float>, collective::PlusOp>(view);

    theSched()->runSchedulerWhile(
      [obj_ptr] { return !obj_ptr->allreduce_done_; });
    obj_ptr->allreduce_done_ = false;
    // StopTimer(obj_ptr->timer_names_.at(payload_size)); // ?
  }
}
#endif

VT_PERF_TEST_MAIN()
