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
#include "vt/collective/reduce/operators/functors/plus_op.h"
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

#ifdef MAGISTRATE_KOKKOS_ENABLED
#include <Kokkos_Core.hpp>
#endif

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr std::array<size_t, 9> const payloadSizes = {
  64, 128, 2048, 16384, 32768, 524288, 1048576, 2097152, 4194304};

struct MyTest : PerfTestHarness {
  MyTest() {
    DisableGlobalTimer();
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

  void initialize() {
    proxy_ = vt::theObjGroup()->getProxy<NodeObj>(this);
  }

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
  std::unordered_map<size_t, std::string> timer_names_= {};
  TestT* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
  bool allreduce_done_ = false;
};

VT_PERF_TEST(MyTest, test_reduce) {
  auto grp_proxy =
    vt::theObjGroup()->makeCollective<NodeObj<MyTest>>("test_allreduce", this, "Reduce -> Bcast vector");

  for (auto payload_size : payloadSizes) {
    data.resize(payload_size, theContext()->getNode() + 1);

    theCollective()->barrier();
    auto* obj_ptr = grp_proxy[my_node_].get();
    StartTimer(obj_ptr->timer_names_.at(payload_size));
    grp_proxy.allreduce<&NodeObj<MyTest>::handlerVec, collective::PlusOp>(data);

    theSched()->runSchedulerWhile([obj_ptr] { return !obj_ptr->allreduce_done_; });
    obj_ptr->allreduce_done_ = false;
  }
}


#if MAGISTRATE_KOKKOS_ENABLED

struct MyTestKokkos : PerfTestHarness {
  MyTestKokkos() {
    DisableGlobalTimer();
  }

  Kokkos::View<float*, Kokkos::HostSpace> view;
};


VT_PERF_TEST(MyTestKokkos, test_reduce_kokkos) {
  auto grp_proxy =
    vt::theObjGroup()->makeCollective<NodeObj<MyTestKokkos>>("test_allreduce", this, "Reduce -> Bcast view");

  for (auto payload_size : payloadSizes) {
    view = Kokkos::View<float*, Kokkos::HostSpace>("view", payload_size);
    Kokkos::parallel_for(
      "InitView", view.extent(0),
      KOKKOS_LAMBDA(const int i) { view(i) = static_cast<float>(my_node_); });

    theCollective()->barrier();
    auto* obj_ptr = grp_proxy[my_node_].get();
    StartTimer(obj_ptr->timer_names_.at(payload_size));

    grp_proxy.allreduce<&NodeObj<MyTestKokkos>::handlerView<float>, collective::PlusOp>(view);

    theSched()->runSchedulerWhile([obj_ptr] { return !obj_ptr->allreduce_done_; });
    obj_ptr->allreduce_done_ = false;
  }
}
#endif

VT_PERF_TEST(MyTest, test_allreduce_rabenseifner) {
  auto proxy = vt::theObjGroup()->makeCollective<NodeObj<MyTest>>(
    "test_allreduce_rabenseifner", this, "Rabenseifner vector"
  );

  for (auto payload_size : payloadSizes) {
    data.resize(payload_size, theContext()->getNode() + 1);

    theCollective()->barrier();
    auto* obj_ptr = proxy[my_node_].get();
    StartTimer(obj_ptr->timer_names_.at(payload_size));
    proxy.allreduce<
      &NodeObj<MyTest>::handlerVec, collective::PlusOp,
      vt::collective::reduce::allreduce::RabenseifnerT>(data);

    theSched()->runSchedulerWhile(
      [obj_ptr] { return !obj_ptr->allreduce_done_; });
    obj_ptr->allreduce_done_ = false;
  }
}

#if MAGISTRATE_KOKKOS_ENABLED
VT_PERF_TEST(MyTestKokkos, test_allreduce_rabenseifner_kokkos) {
  auto proxy = vt::theObjGroup()->makeCollective<NodeObj<MyTestKokkos>>(
    "test_allreduce_rabenseifner", this, "Rabenseifner view"
  );

  for (auto payload_size : payloadSizes) {
    view = Kokkos::View<float*, Kokkos::HostSpace>("view", payload_size);

    theCollective()->barrier();
    auto* obj_ptr = proxy[my_node_].get();
    StartTimer(obj_ptr->timer_names_.at(payload_size));

    proxy.allreduce<
      &NodeObj<MyTestKokkos>::handlerView<float>, collective::PlusOp,
      vt::collective::reduce::allreduce::RabenseifnerT>(view);

    theSched()->runSchedulerWhile([obj_ptr] { return !obj_ptr->allreduce_done_; });
    obj_ptr->allreduce_done_ = false;
  }
}
#endif // MAGISTRATE_KOKKOS_ENABLED

VT_PERF_TEST(MyTest, test_allreduce_recursive_doubling) {
  auto proxy = vt::theObjGroup()->makeCollective<NodeObj<MyTest>>(
    "test_allreduce_recursive_doubling", this, "Recursive doubling vector"
  );

  for (auto payload_size : payloadSizes) {
    data.resize(payload_size, theContext()->getNode() + 1);

    theCollective()->barrier();
    auto* obj_ptr = proxy[my_node_].get();
    StartTimer(obj_ptr->timer_names_.at(payload_size));

    proxy.allreduce<
      &NodeObj<MyTest>::handlerVec, collective::PlusOp,
      vt::collective::reduce::allreduce::RecursiveDoublingT>(data);

    theSched()->runSchedulerWhile(
      [obj_ptr] { return !obj_ptr->allreduce_done_; });
    obj_ptr->allreduce_done_ = false;
  }
}

#if MAGISTRATE_KOKKOS_ENABLED
VT_PERF_TEST(MyTestKokkos, test_allreduce_recursive_doubling_kokkos) {
  auto proxy = vt::theObjGroup()->makeCollective<NodeObj<MyTestKokkos>>(
    "test_allreduce_rabenseifner", this, "Recursive doubling view"
  );

  for (auto payload_size : payloadSizes) {
    view = Kokkos::View<float*, Kokkos::HostSpace>("view", payload_size);

    theCollective()->barrier();
    auto* obj_ptr = proxy[my_node_].get();
    StartTimer(obj_ptr->timer_names_.at(payload_size));

    proxy.allreduce<
      &NodeObj<MyTestKokkos>::handlerView<float>, collective::PlusOp,
      vt::collective::reduce::allreduce::RecursiveDoublingT>(view);

    theSched()->runSchedulerWhile([obj_ptr] { return !obj_ptr->allreduce_done_; });
    obj_ptr->allreduce_done_ = false;
  }
}
#endif // MAGISTRATE_KOKKOS_ENABLED

namespace test_group{
  bool group_allreduce_done = false;
  std::unordered_map<size_t, std::string> timer_names = {};
  MyTest* parent = {};
}

void allreduce_group_han(std::vector<int32_t> result){
    // std::string result_s = "";
    // for(auto val : result){
    //   result_s.append(fmt::format("{} ", val));
    // }
    // fmt::print(
    //   "[{}]: Allreduce handler (Values=[{}])\n",
    //   theContext()->getNode(), result_s
    // );
    test_group::parent->StopTimer(test_group::timer_names.at(result.size()));
    test_group::group_allreduce_done = true;
}

VT_PERF_TEST(MyTest, test_allreduce_group_rabenseifner) {
    for (auto const payload_size : payloadSizes) {
    test_group::timer_names[payload_size] =
      fmt::format("Group vector {}", payload_size);
    }
    test_group::parent = this;
    bool group_ready = false;
    auto const is_odd = my_node_ % 2 == 1;

    auto g = vt::theGroup()->newGroupCollective(
      is_odd, [&](GroupType) { group_ready = true; });
    theSched()->runSchedulerWhile([&] { return !group_ready; });

    for (auto payload_size : payloadSizes) {
    std::vector<int32_t> payload(payload_size, theContext()->getNode());

    StartTimer(test_group::timer_names.at(payload_size));
    vt::theGroup()->allreduce<allreduce_group_han, collective::PlusOp>(
      g, std::move(payload));

    theSched()->runSchedulerWhile(
      [is_odd] { return is_odd and !test_group::group_allreduce_done; });
    test_group::group_allreduce_done = false;
    }
}

struct Hello : vt::Collection<Hello, vt::Index1D> {
    Hello() {
      for (auto const payload_size : payloadSizes) {
      timer_names_[payload_size] = fmt::format("Collection {}", payload_size);
      }
    }

    void finalHan(std::vector<int32_t> result) {
      // std::string result_s = "";
      // for(auto val : result){
      //   result_s.append(fmt::format("{} ", val));
      // }
      // fmt::print(
      //   "[{}]: Allreduce handler (Values=[{}]), idx={}\n",
      //   theContext()->getNode(), result_s, getIndex().x()
      // );

      col_send_done_ = true;
      parent_->StopTimer(timer_names_.at(result.size()));
    }

    void handler(size_t payload_size) {
      auto proxy = this->getCollectionProxy();

      std::vector<int32_t> payload(payload_size, getIndex().x());
      parent_->StartTimer(timer_names_.at(payload_size));
      proxy.allreduce_h<&Hello::finalHan, collective::PlusOp>(
        std::move(payload));
    }

    bool col_send_done_ = false;
    std::unordered_map<size_t, std::string> timer_names_ = {};
    MyTest* parent_ = {};
};

VT_PERF_TEST(MyTest, test_allreduce_collection_rabenseifner) {
    auto range = vt::Index1D(int32_t{num_nodes_});
    auto proxy = vt::makeCollection<Hello>("test_collection_send")
                   .bounds(range)
                   .bulkInsert()
                   .wait();

    auto const thisNode = vt::theContext()->getNode();
    auto const nextNode = (thisNode + 1) % num_nodes_;

    theCollective()->barrier();

    auto const num_elms_per_node = 1;
    auto const elm = thisNode * num_elms_per_node;

    proxy[elm].tryGetLocalPtr()->parent_ = this;
    proxy.broadcastCollective<&Hello::handler>(payloadSizes.front());
    theSched()->runSchedulerWhile(
      [&] { return !proxy[elm].tryGetLocalPtr()->col_send_done_; });
    // for (auto payload_size : payloadSizes) {
    //   proxy.broadcastCollective<&Hello::handler>(payload_size);

    //   // We run 1 coll elem per node, so it should be ok
    //   theSched()->runSchedulerWhile([&] { return !proxy[elm].tryGetLocalPtr()->col_send_done_; });
    //   proxy[elm].tryGetLocalPtr()->col_send_done_ = false;
    // }
}

VT_PERF_TEST_MAIN()
