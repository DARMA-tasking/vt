/*
//@HEADER
// ************************************************************************
//
//                          lb_iter.cc
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

#include <cstdlib>

using namespace ::vt;
using namespace ::vt::collective;

static constexpr int32_t const default_num_elms = 64;
static int32_t num_iter = 8;

struct IterCol : Collection<IterCol,Index1D> {
  IterCol() = default;

private:
  int data_1 = 29;
public:
  float data_2 = 2.4f;
};

struct IterMsg : CollectionMessage<IterCol> {
  IterMsg() = default;
  explicit IterMsg(int64_t const in_work_amt, int64_t const in_iter)
    : iter_(in_iter), work_amt_(in_work_amt)
  { }

  int64_t iter_ = 0;
  int64_t work_amt_ = 0;
};

struct IterReduceMsg : collective::ReduceTMsg<NoneType> {};

static int32_t cur_iter = 0;
static TimeType cur_time = 0;
static CollectionIndexProxy<IterCol,Index1D> proxy = {};

static void startIter(int32_t const iter);

struct FinishedIter {
  void operator()(IterReduceMsg* raw_msg) {
    auto msg = promoteMsg(raw_msg);
    auto const new_time = ::vt::timing::Timing::getCurrentTime();
    ::fmt::print(
      "finished iteration: iter={},time={}\n",
      cur_iter,new_time-cur_time
    );
    cur_iter++;
    cur_time = new_time;
    if (cur_iter < num_iter) {
      theCollection()->nextPhase<IterCol>(proxy,cur_iter-1,[=]{
        startIter(cur_iter);
      });
    }
  }
};

static double weight = 1.0f;

static void iterWork(IterMsg* msg, IterCol* col) {
  double val = 0.1f;
  double val2 = 0.4f * msg->work_amt_;
  auto const idx = col->getIndex().x();
  //auto const iter = msg->iter_;
  //::fmt::print("proc={}, idx={}, iter={}\n", theContext()->getNode(),idx,iter);
  int64_t const max_work = 1000 * weight;
  int64_t const mid_work = 100 * weight;
  int64_t const min_work = 1 * weight;
  int const x = idx < 8 ? max_work : (idx > 40 ? mid_work : min_work);
  for (int i = 0; i < 10000 * x; i++) {
    val *= val2 + i*29.4;
    val2 += 1.0;
  }
  col->data_2 += val + val2;

  auto reduce_msg = makeSharedMessage<IterReduceMsg>();
  theCollection()->reduceMsg<
    IterCol,
    IterReduceMsg,
    IterReduceMsg::template msgHandler<
      IterReduceMsg, collective::PlusOp<collective::NoneType>, FinishedIter
    >
  >(col->getCollectionProxy(), reduce_msg);
}

static void startIter(int32_t const iter) {
  auto msg = makeSharedMessage<IterMsg>(10,iter);
  proxy.broadcast<IterMsg,iterWork>(msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }
  if (argc > 2) {
    weight = atoi(argv[2]);
  }
  if (argc > 3) {
    num_iter = atoi(argv[3]);
  }

  if (this_node == 0) {
    ::fmt::print(
      "lb_iter: elms={}, weight={}, num_iter={}\n",
      num_elms, weight, num_iter
    );

    auto const& range = Index1D(num_elms);
    proxy = theCollection()->construct<IterCol>(range);
    cur_time = ::vt::timing::Timing::getCurrentTime();
    startIter(0);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
