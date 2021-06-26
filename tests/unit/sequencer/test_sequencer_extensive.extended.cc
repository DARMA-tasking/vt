/*
//@HEADER
// *****************************************************************************
//
//                     test_sequencer_extensive.extended.cc
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

#include <gtest/gtest.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "test_helpers.h"

#include "vt/sequence/sequencer.h"

using namespace vt;
using namespace vt::tests::unit;

#if DEBUG_TEST_HARNESS_PRINT
#define DEBUG_PRINT_SEQ(ORDER, CUR, LABEL)                              \
  do {                                                                  \
    auto seq_id = theSeq()->getCurrentSeq();                            \
    fmt::print(                                                         \
      "debug ({}): seq_id={}, ordering={} -- cur={} --\n",              \
      (LABEL), seq_id, (ORDER).load(), (CUR)                            \
    );                                                                  \
  } while (false);
#define VT_DEBUG_PRINT(str, ...)                                           \
  do { fmt::print(str, __VA_ARGS__); } while (false);
#else
#define DEBUG_PRINT_SEQ(ORDER, CUR, LABEL)
#define VT_DEBUG_PRINT(str, ...)
#endif

namespace vt { namespace tests { namespace unit { namespace extensive {

static constexpr SeqType const FinalizeAtomicValue = -1;
static constexpr SeqType const ResetAtomicValue = -2;

using CountType = uint32_t;
using ParamType = std::tuple<CountType, CountType, CountType, CountType, CountType>;
using ParamContainerType = std::vector<ParamType>;

static constexpr CountType const param_size = std::tuple_size<ParamType>::value;

using NumWaitsMsg = WaitInfoMsg<vt::ShortMessage, uint32_t, param_size>;

static constexpr CountType const max_num_waits = 4;
static constexpr CountType const max_num_waits_before = 2;
static constexpr CountType const max_num_waits_after = 2;
static constexpr CountType const max_num_segs = 8;
static constexpr CountType const max_seq_depth = 8;

#define CALL_EXPAND(SEQ_HAN, SEQ_FN, NODE, MSG_TYPE, ___, IS_TAG)       \
  do {                                                                  \
    auto const& node = theContext()->getNode();                         \
    auto param = GetParam();                                            \
    CountType const& wait_cnt = std::get<0>(param);                     \
    CountType const& wait_pre = std::get<1>(param);                     \
    CountType const& wait_post = std::get<2>(param);                    \
    CountType const& seg_cnt = std::get<3>(param);                      \
    CountType const& depth = std::get<4>(param);                        \
    runInEpochCollective([=]{                                           \
      if (node == (NODE)) {                                             \
        SeqType const& seq_id = theSeq()->nextSeq();                    \
        SEQ_FN(ResetAtomicValue);                                       \
        theSeq()->sequenced(seq_id, (SEQ_FN));                          \
      } else if (node == 1) {                                           \
        CountType in[param_size] = {                                    \
          wait_cnt, wait_pre, wait_post, seg_cnt, depth                 \
        };                                                              \
        auto msg = makeMessage<NumWaitsMsg>(in);                        \
        theMsg()->sendMsg<NumWaitsMsg, numWaitHan>(                     \
          (NODE), msg                                                   \
        );                                                              \
        auto const total = (wait_cnt * seg_cnt) + wait_pre + wait_post; \
        for (CountType i = 0; i < total; i++) {                         \
          TagType const tag = (IS_TAG) ? i+1 : no_tag;                  \
          auto nmsg = makeMessage<MSG_TYPE>();                          \
          theMsg()->sendMsg<MSG_TYPE, SEQ_HAN>(                         \
            (NODE), nmsg, tag                                           \
          );                                                            \
        }                                                               \
      }                                                                 \
    });                                                                 \
    if (node == (NODE)) {                                               \
      SEQ_FN(FinalizeAtomicValue);                                      \
    }                                                                   \
  } while (false);                                                      \

#define FN_APPLY(SEQ_HAN, SEQ_FN, NODE, MSG_TYPE, ___, IS_TAG)          \
  static void SEQ_FN(SeqType const& seq_id) {                           \
    static std::atomic<OrderType> seq_ordering_{};                      \
                                                                        \
    static uint32_t num_waits = 0;                                      \
    static uint32_t nwaits_pre = 0;                                     \
    static uint32_t nwaits_post = 0;                                    \
    static uint32_t num_segs = 0;                                       \
    static uint32_t depth = 0;                                          \
    static uint32_t const nwait_offset = 2;                             \
                                                                        \
    if (seq_id == FinalizeAtomicValue || seq_id == ResetAtomicValue) {  \
      if (seq_id == FinalizeAtomicValue) {                              \
        VT_DEBUG_PRINT(                                                    \
          "num_waits+1={},seq_ordering_={}\n",                          \
          num_waits+1,seq_ordering_.load()                              \
        );                                                              \
        EXPECT_EQ(                                                      \
          seq_ordering_++,                                              \
          (num_waits * num_segs) +                                      \
          nwaits_pre + nwaits_post + nwait_offset                       \
        );                                                              \
      } else if (seq_id == ResetAtomicValue) {                          \
        seq_ordering_.store(0);                                         \
      }                                                                 \
      return;                                                           \
    }                                                                   \
                                                                        \
    EXPECT_EQ(seq_ordering_++, 0U);                                     \
                                                                        \
    theSeq()->wait_closure<NumWaitsMsg, numWaitHan>(                    \
      no_tag, [](NumWaitsMsg* m){                                       \
        EXPECT_EQ(seq_ordering_++, 1U);                                 \
        num_waits = m->info[0];                                         \
        nwaits_pre = m->info[1];                                        \
        nwaits_post = m->info[2];                                       \
        num_segs = m->info[3];                                          \
        depth = m->info[4];                                             \
      }                                                                 \
    );                                                                  \
    VT_DEBUG_PRINT("before pre sequence {}\n", seq_ordering_);             \
                                                                        \
    theSeq()->sequenced([=]{                                            \
      for (uint32_t wb = 0; wb < nwaits_pre; wb++) {                    \
        theSeq()->wait_closure<MSG_TYPE, SEQ_HAN>(                      \
          no_tag, [=](MSG_TYPE* msg){                                   \
            CountType const this_wait = wb + nwait_offset;              \
            EXPECT_EQ(seq_ordering_++, this_wait);                      \
            DEBUG_PRINT_SEQ(seq_ordering_, this_wait, "seq-pre");       \
          }                                                             \
        );                                                              \
      }                                                                 \
      VT_DEBUG_PRINT("after pre sequence {}\n", seq_ordering_);            \
      VT_DEBUG_PRINT("before mid sequence {}\n", seq_ordering_);           \
                                                                        \
      for (uint32_t nseg = 0; nseg < num_segs; nseg++) {                \
        theSeq()->sequenced([=]{                                        \
          VT_DEBUG_PRINT("inside mid sequence:nseg={},depth={}\n",nseg,depth); \
          VT_DEBUG_PRINT("nseg={}:num_waits={}\n",nseg,num_waits);         \
          DEBUG_PRINT_SEQ(seq_ordering_, 0, "start-sequenced");         \
          seqDepth(depth, [=]{                                          \
            for (uint32_t w = 0; w < num_waits; w++) {                  \
              theSeq()->wait_closure<MSG_TYPE, SEQ_HAN>(                \
                no_tag, [=](MSG_TYPE* msg){                             \
                  CountType const this_wait =                           \
                    (nseg * num_waits) + w + nwaits_pre + nwait_offset; \
                  DEBUG_PRINT_SEQ(seq_ordering_, this_wait, "bexpect seq-main"); \
                  EXPECT_EQ(seq_ordering_++, this_wait);                \
                  DEBUG_PRINT_SEQ(seq_ordering_, this_wait, "seq-main"); \
                }                                                       \
              );                                                        \
            }                                                           \
          });                                                           \
        });                                                             \
      }                                                                 \
                                                                        \
      VT_DEBUG_PRINT("after mid sequence {}\n", seq_ordering_);            \
      VT_DEBUG_PRINT("before post sequence {}\n", seq_ordering_);          \
                                                                        \
      for (uint32_t wa = 0; wa < nwaits_post; wa++) {                   \
        theSeq()->wait_closure<MSG_TYPE, SEQ_HAN>(                      \
          no_tag, [=](MSG_TYPE* msg){                                   \
            CountType const this_wait =                                 \
              (num_segs * num_waits) + nwaits_pre + wa + nwait_offset;  \
            DEBUG_PRINT_SEQ(seq_ordering_, this_wait, "seq-post");      \
            EXPECT_EQ(seq_ordering_++, this_wait);                      \
          }                                                             \
        );                                                              \
      }                                                                 \
    });                                                                 \
    VT_DEBUG_PRINT("after post sequence {}\n", seq_ordering_);             \
  }                                                                     \


static inline ParamContainerType make_values() {

  /**
   * This is a special case where this particular test heavily underperforms
   * when running with oversubscribed number of nodes, even when the test body is
   * empty or test is skipped (using GTEST_SKIP). This is probably due to the
   * number of test cases generated by this function (672 to be exact).
   *
   * So in this case we want to disable the parametrized generation when
   * we oversubscribe
  */
  if(isOversubscribed()){
    return ParamContainerType{};
  }

  ParamContainerType testing_values;
  for (CountType nwaits = 1; nwaits < max_num_waits; nwaits++) {
    for (CountType nsegs = 1; nsegs < max_num_segs; nsegs++) {
      for (CountType d = 0; d < max_seq_depth; d++) {
        for (CountType wb = 0; wb < max_num_waits_before; wb++) {
          for (CountType wa = 0; wa < max_num_waits_after; wa++) {
            testing_values.emplace_back(std::make_tuple(nwaits,nsegs,d,wb,wa));
          }
        }
      }
    }
  }
  return testing_values;
}

struct TestSequencerExtensive : TestParallelHarnessParam<ParamType> {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using NumWaitsMsg2 = NumWaitsMsg;
  using OrderType = uint32_t;

  virtual void SetUp() {
    TestParallelHarnessParam<ParamType>::SetUp();
  }

  SEQUENCE_REGISTER_HANDLER(TestSequencerExtensive::NumWaitsMsg2, numWaitHan);
  SEQUENCE_REGISTER_HANDLER(TestSequencerExtensive::TestMsg, testSeqHan1);

  FN_APPLY(testSeqHan1, testSeqFn1, 0, TestMsg, 2, false);

  static void seqDepth(int depth, std::function<void()> fn) {
    if (depth == 0) {
      fn();
    } else {
      theSeq()->sequenced([=]{
        return seqDepth(depth-1, fn);
      });
    }
  }
};

TEST_P(TestSequencerExtensive, test_wait_1) {
  SET_NUM_NODES_RANGE_CONSTRAINT(2, CMAKE_DETECTED_MAX_NUM_NODES);

  CALL_EXPAND(testSeqHan1, testSeqFn1, 0, TestMsg, 2, false);
}

INSTANTIATE_TEST_SUITE_P(
  test_sequencer_extensive, TestSequencerExtensive,
  testing::ValuesIn(make_values())
);

}}}} // end namespace vt::tests::unit::extensive
