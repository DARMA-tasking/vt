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

#include <vt/transport.h>
#include <Kokkos_Core.hpp>

static constexpr std::size_t PAYLOAD_SIZE = 2048;

struct MyTest {
  MyTest() { data_.reserve(PAYLOAD_SIZE); }

  std::vector<int32_t> data_;
};

template <typename TestT>
struct NodeObj {
  explicit NodeObj(TestT* test_obj, const std::string&)
    : test_obj_(test_obj) { }

  void initialize() { proxy_ = vt::theObjGroup()->getProxy<NodeObj>(this); }

  void handlerVec(const std::vector<int32_t>& vec) { allreduce_done_ = true; }

  template <typename Scalar>
  void handlerView(Kokkos::View<Scalar*, Kokkos::HostSpace> view) {
    allreduce_done_ = true;
  }

  TestT* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
  bool allreduce_done_ = false;
};

// VT_PERF_TEST(MyTest, test_mpi_allreduce) {
//   for (auto payload_size : payloadSizes) {
//     data.resize(payload_size, theContext()->getNode() + 1);
//     std::vector<int> result(payload_size);
//     MPI_Barrier(MPI_COMM_WORLD);
//     StartTimer("MPI_Allreduce vector " + std::to_string(payload_size));
//     MPI_Allreduce(
//       data.data(), result.data(), payload_size, MPI_INT, MPI_SUM,
//       MPI_COMM_WORLD);
//     StopTimer("MPI_Allreduce vector " + std::to_string(payload_size));
//   }
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

struct MyTestKokkos {
  MyTestKokkos() = default;
  void TestFunc();

  Kokkos::View<float*, Kokkos::HostSpace> view_;
};

// VT_PERF_TEST(MyTestKokkos, test_mpi_allreduce_kokkos) {
//   for (auto payload_size : payloadSizes) {
//     view = Kokkos::View<float*, Kokkos::HostSpace>("view", payload_size);
//     auto result =
//       Kokkos::View<float*, Kokkos::HostSpace>("result", payload_size);
//     MPI_Barrier(MPI_COMM_WORLD);
//     StartTimer("MPI_Allreduce view " + std::to_string(payload_size));
//     MPI_Allreduce(
//       view.data(), result.data(), payload_size, MPI_FLOAT, MPI_SUM,
//       MPI_COMM_WORLD);
//     StopTimer("MPI_Allreduce view " + std::to_string(payload_size));
//   }
// }

// VT_PERF_TEST(MyTestKokkos, test_reduce_kokkos) {
void MyTestKokkos::TestFunc() {
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj<MyTestKokkos>>(
    "test_allreduce", this, "Reduce -> Bcast view");
  auto my_node_ = vt::theContext()->getNode();

  view_ = Kokkos::View<float*, Kokkos::HostSpace>("view", PAYLOAD_SIZE);
  for (uint32_t i = 0; i < view_.extent(0); ++i) {
    view_(i) = static_cast<float>(my_node_);
  }

  vt::theCollective()->barrier();
  auto* obj_ptr = grp_proxy[my_node_].get();

  grp_proxy.allreduce<
    &NodeObj<MyTestKokkos>::handlerView<float>, vt::collective::PlusOp>(view_);

  vt::theSched()->runSchedulerWhile(
    [obj_ptr] { return !obj_ptr->allreduce_done_; });
  obj_ptr->allreduce_done_ = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
  Kokkos::ScopeGuard guard(argc, argv);

  // MPI_Init(&argc, &argv);
  // int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  vt::initialize(argc, argv);
  vt::NodeType this_node = vt::theContext()->getNode();

  constexpr auto num_runs = 10000;
  MyTestKokkos test;

  for (uint32_t run_num = 1; run_num <= num_runs; ++run_num) {
    test.TestFunc();
    vt::theSched()->runSchedulerWhile([] { return !vt::rt->isTerminated(); });
  }

  // MPI_Finalize();
  vt::finalize();

  return 0;
}
