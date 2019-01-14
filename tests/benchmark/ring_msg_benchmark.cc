/*
//@HEADER
// ************************************************************************
//
//                      ring_msg_benchmark.cc
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

#include "vt/transport.h"
#include "benchmark.h"

#include <fmt/format.h>

#include <string>
#include <memory>
#include <cassert>

namespace vt { namespace tests { namespace benchmarks {

struct RingBenchmark final : Benchmark {

  RingBenchmark(
    std::string const& in_name,
    int64_t const& in_user_k = min_k,
    int64_t const& in_max_d = max_d,
    std::string unit = ""
  ) : Benchmark(in_name, in_user_k, in_max_d, unit)
  {
    static_bench_ = this;
  }

  static RingBenchmark* static_bench_;

private:
 /*
  * Handler for the BlockMsg<d> arriving for n_th time: trigger send to next
  */
  template <int64_t d>
  static void blockHandler(BlockDataMsg<d>* msg) {
    static_bench_->sendToNext<d>(msg->k());
  }

  template <int64_t d>
  LessD<d,max_d> finished(int64_t k) {
    bool const last_iter = d >= max_user_d_;
    print(d, k,last_iter);
    if (not last_iter) {
      startRing<d+1>(cur_k_);
    }
  }

  template <int64_t d>
  DoneD<d,max_d> finished(int64_t k) {
    print(d,k,true);
  }

  template <int64_t d>
  void sendToNext(int64_t k) {
    using MsgType = BlockDataMsg<d>;
    static int32_t cur_times = 0;
    if (cur_times == 0) {
      start_time_ = vt::timing::Timing::getCurrentTime();
    }
    if (cur_times < k) {
      auto msg = vt::makeMessage<MsgType>(k);
      vt::theMsg()->sendMsg<MsgType, RingBenchmark::blockHandler<d>>(
        ring_node_,msg.get()
      );
      cur_times++;
    } else {
      cur_times = 0;
      finished<d>(k);
    }
  }

  template <int64_t d>
  void startRing(int64_t k) {
    sendToNext<d>(k);
  }

public:
  void start() override {
    if (this_node_ ==  0) {
      startRing<min_d>(cur_k_);
    }
  }
};

/*static*/ RingBenchmark* RingBenchmark::static_bench_ = nullptr;

}}} /* end namespace vt::tests::benchmarks */

int main(int argc, char** argv) {
  auto rt = vt::initialize(argc, argv);

  if (vt::theContext()->getNumNodes() == 1) {
    vt::abort("At least 2 ranks required");
  }

  int64_t     const in_user_k = argc > 1 ? atoi(argv[1]) : 0;
  int64_t     const in_max_d  = argc > 2 ? atoi(argv[2]) : 0;
  bool        const in_e_not  = argc > 3 ? atoi(argv[3]) > 0 : false;
  std::string const in_unit   = argc > 4 ? argv[4] : "ms";

  auto ring = std::make_unique<vt::tests::benchmarks::RingBenchmark>(
    "AM Handler Ring", in_user_k, in_max_d, in_unit
  );
  vt::theCollective()->barrier();
  ring->start();
  vt::theCollective()->barrier();

  while (!rt->isTerminated()) {
    vt::runScheduler();
  }

  return 0;
}

