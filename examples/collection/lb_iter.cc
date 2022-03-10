/*
//@HEADER
// *****************************************************************************
//
//                                  lb_iter.cc
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

static constexpr int32_t const default_num_elms = 64;
static int32_t num_iter = 8;

struct IterCol : vt::Collection<IterCol, vt::Index1D> {
  IterCol() = default;

  struct IterMsg : vt::CollectionMessage<IterCol> {
    IterMsg() = default;
    IterMsg(
      int64_t const in_work_amt, int64_t const in_iter, int64_t const subphase
    )
      : iter_(in_iter), work_amt_(in_work_amt), subphase_(subphase)
    { }

    int64_t iter_ = 0;
    int64_t work_amt_ = 0;
    int64_t subphase_ = 0;
  };

  void iterWork(IterMsg* msg);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<IterCol, vt::Index1D>::serialize(s);
    s | data_2;
  }

private:
  float data_2 = 2.4f;
};

static double weight = 1.0f;

void IterCol::iterWork(IterMsg* msg) {
  this->stats_.setSubPhase(msg->subphase_);
  double val = 0.1f;
  double val2 = 0.4f * msg->work_amt_;
  auto const idx = getIndex().x();
  int64_t const max_work = 1000 * weight;
  int64_t const mid_work = 100 * weight;
  int64_t const min_work = 1 * weight;
  int const x = idx < 8 ? max_work : (idx > 40 ? mid_work : min_work);
  for (int i = 0; i < 10000 * x; i++) {
    val *= val2 + i*29.4;
    val2 += 1.0;
  }
  data_2 += val + val2;

  auto const this_node = vt::theContext()->getNode();
  if (idx == 0) {
    fmt::print("{}: iterWork: idx={}\n", this_node, getIndex());
  }
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

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

  vt::NodeType this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    fmt::print(
      "lb_iter: elms={}, weight={}, num_iter={}\n",
      num_elms, weight, num_iter
    );
  }

  auto range = vt::Index1D(num_elms);
  auto proxy = vt::makeCollection<IterCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  for (int i = 0; i < num_iter; i++) {
    auto cur_time = vt::timing::getCurrentTime();

    vt::runInEpochCollective([=]{
      proxy.broadcastCollective<IterCol::IterMsg,&IterCol::iterWork>(10, i, 0);
    });
    vt::runInEpochCollective([=]{
      proxy.broadcastCollective<IterCol::IterMsg,&IterCol::iterWork>(5,  i, 1);
    });
    vt::runInEpochCollective([=]{
      proxy.broadcastCollective<IterCol::IterMsg,&IterCol::iterWork>(15, i, 2);
    });

    auto total_time = vt::timing::getCurrentTime() - cur_time;
    if (this_node == 0) {
      fmt::print("iteration: iter={},time={}\n", i, total_time);
    }

    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();
  return 0;
}
