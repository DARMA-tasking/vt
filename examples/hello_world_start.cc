/*
//@HEADER
// *****************************************************************************
//
//                             hello_world_start.cc
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

#include "vt/transport.h"
#include "vt/utils/tls/tls.h"

#include <cstdlib>
#include <cstdio>
#include <vector>

using namespace vt;
using namespace vt::vrt;
using namespace vt::mapping;

#define DEBUG_START_EXAMPLE 0

#define DEBUG_PRINTER_START(str, ...)                                   \
  do{                                                                   \
    fmt::print(                                                         \
      "node={},worker={}: " str,                                        \
      theContext()->getNode(), theContext()->getWorker(), __VA_ARGS__   \
    );                                                                  \
  } while (false);

#if DEBUG_START_EXAMPLE
  #define DEBUG_PRINT_START(str, ...) DEBUG_PRINTER_START(str, __VA_ARGS__)
#else
  #define DEBUG_PRINT_START(str, ...)
#endif

struct ProxyMsg : vt::vrt::VirtualMessage {
  using MessageParentType = vt::vrt::VirtualMessage;
  vt_msg_serialize_required(); // by proxies

  std::vector<VirtualProxyType> proxies;

  ProxyMsg() = default;
  ProxyMsg(std::vector<VirtualProxyType> const& in_proxies)
    : VirtualMessage(), proxies(in_proxies)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | proxies;
  }
};

struct WorkMsg : vt::vrt::VirtualMessage {
  using MessageParentType = vt::vrt::VirtualMessage;
  vt_msg_serialize_required(); // by work_vec
  std::vector<double> work_vec;

  WorkMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | work_vec;
  }
};

struct TestVC;
static void doWorkRight(WorkMsg* msg, TestVC* vc);

static ::vt::util::atomic::AtomicType<int> num_work_finished = {0};

DeclareInitTLS(int, tls_work, 0);

struct TestVC : vt::vrt::VirtualContext {
  VirtualProxyType my_proxy = no_vrt_proxy;
  VirtualProxyType right_proxy = no_vrt_proxy;
  int my_index = 0;
  int work_amt = 0;

  double stored_val = 0.0;

  TestVC(int const in_my_index, int const in_work_amt)
    : my_index(in_my_index), work_amt(in_work_amt)
  {
    DEBUG_PRINT_START(
      "node={}, worker={}: {}: constructing TestVC: my_index={}, work amt={}\n",
      theContext()->getNode(), theContext()->getWorker(), this, my_index,
      work_amt
    );
  }

  void getRightProxy(ProxyMsg* msg) {
    my_proxy = msg->proxies[my_index];

    DEBUG_PRINT_START(
      "my_proxy={}, index={}, getProxy()={}, ptr={}\n",
      my_proxy, my_index, getProxy(), this
    );

    vtAssertExpr(my_proxy == getProxy());

    auto const to_right = my_index+1;
    bool const has_right = to_right < msg->proxies.size();

    if (has_right) {
      right_proxy = msg->proxies[to_right];
    }

    DEBUG_PRINT_START(
      "my_proxy={}, proxies size={}, right={} vc={} has_right={}, "
      "right proxy={}\n",
      getProxy(), msg->proxies.size(), to_right, this, print_bool(has_right),
      right_proxy
    );

    if (has_right) {
      auto msg = makeSharedMessage<WorkMsg>();
      for (auto i = 0; i < my_index + 29; i++) {
        msg->work_vec.push_back(static_cast<double>(work_amt));
      }

      theVirtualManager()->sendSerialMsg<TestVC, WorkMsg, doWorkRight>(
        right_proxy, msg
      );
    }
  }

  void doWork(WorkMsg* msg) {
    DEBUG_PRINT_START(
      "my_proxy={}, doing work size={}\n", my_proxy, msg->work_vec.size()
    );

    double val = 1.23;
    for (auto&& elm : msg->work_vec) {
      val += elm * 3.14159;
    }
    stored_val = val;

    num_work_finished++;
    AccessTLS(tls_work)++;

    DEBUG_PRINT_START(
      "my_proxy={}, finished work val={}\n", my_proxy, stored_val
    );
  }
};

static void doWorkRight(WorkMsg* msg, TestVC* vc) {
  DEBUG_PRINT_START(
    "doWorkRight: msg->work_vec.size()={}, vc={}\n", msg->work_vec.size(), vc
  );
  vc->doWork(msg);
}

static void proxyHan(ProxyMsg* msg, TestVC* vc) {
  DEBUG_PRINT_START("proxyHan: msg->proxies={}, vc={}\n", msg->proxies.size(), vc);
  vc->getRightProxy(msg);
}

struct MakeMainMsg : vt::Message { };
static void makeMain(MakeMainMsg* msg);

struct MainVC : vt::vrt::MainVirtualContext {
  int my_data = -1;

  MainVC() : my_data(29) {
    DEBUG_PRINT_START("constructing MainVC: data={}\n", my_data);

    if (theContext()->getNode() == 0) {
      auto msg = makeSharedMessage<MakeMainMsg>();
      theMsg()->broadcastMsg<MakeMainMsg, makeMain>(msg);
    }

    theTerm()->addAction([]{
      auto const num_work_units = num_work_finished.load();
      DEBUG_PRINTER_START("num_work_units={}\n", num_work_units);

      // theWorkerGrp()->enqueueAllWorkers([]{
      //   auto const num_work_units = AccessTLS(tls_work);
      //   DEBUG_PRINTER_START("tls work_units={}\n", num_work_units);
      // });
    });

    std::vector<VirtualProxyType> proxies;

    for (auto i = 0; i < my_data; i++) {
      //auto proxy = theVirtualManager()->makeVirtual<TestVC>(i, i * 82773);
      auto proxy = theVirtualManager()->makeVirtualMap<TestVC, randomSeedMapCore>(
        i, i * 82773
      );
      proxies.push_back(proxy);

      DEBUG_PRINT_START(
        "{}: constructing proxy {}, size={}, work={}\n",
        i, proxy, proxies.size(), i*82773
      );
    }

    int i = 0;
    for (auto&& elm : proxies) {
      ProxyMsg* msg = makeSharedMessage<ProxyMsg>(proxies);
      DEBUG_PRINT_START("{}: sending to proxy {}\n", i, elm);
      theVirtualManager()->sendSerialMsg<TestVC, ProxyMsg, proxyHan>(elm, msg);
      i++;
    }
  }
};

static void makeMain(MakeMainMsg* msg) {
  theVirtualManager()->makeVirtual<MainVC>();
}


VT_REGISTER_MAIN_CONTEXT(MainVC);
