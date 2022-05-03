/*
//@HEADER
// *****************************************************************************
//
//                         test_subphase_management.cc
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

#include "test_parallel_harness.h"
#include "test_helpers.h"

#include <vt/vrt/collection/manager.h>
#include <vt/messaging/collection_chain_set.h>
#include <vt/phase/phase_manager.h>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace subphase {

static constexpr int const num_elms = 32;
static constexpr int const num_phases = 3;

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  MyCol() = default;

  explicit MyCol(checkpoint::SERIALIZE_CONSTRUCT_TAG) {}
};

struct MyMsg : vt::CollectionMessage<MyCol> {
  explicit MyMsg(vt::PhaseType expected_subphase) {
    expected_subphase_ = expected_subphase;
  }

  vt::PhaseType expected_subphase_ = vt::no_lb_phase;
};

void colHandler(MyMsg* msg, MyCol* col) {
  EXPECT_EQ(vt::thePhase()->getCurrentSubphase(), msg->expected_subphase_);
}

struct MyObjgrp {
  MyObjgrp() = default;
  MyObjgrp(const MyObjgrp& obj) = delete;
  MyObjgrp& operator=(const MyObjgrp& obj) = delete;
  MyObjgrp(MyObjgrp&&) noexcept = default;
  MyObjgrp& operator=(MyObjgrp&& obj) noexcept = default;
  ~MyObjgrp() = default;

  void handler(MyMsg* msg) { }
};

using TestSubphaseManagement = TestParallelHarness;

TEST_F(TestSubphaseManagement, test_no_subphases) {
  auto range = vt::Index1D(num_elms);

  auto this_node = theContext()->getNode();

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runInEpochCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });

    runInEpochCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });

    runInEpochCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_subphase_collective) {
  auto range = vt::Index1D(num_elms);

  auto this_node = theContext()->getNode();

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });
    ++expected_subphase;

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });
    ++expected_subphase;

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });
    ++expected_subphase;

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_subphase_collective_with_non_1) {
  auto range = vt::Index1D(num_elms);

  auto this_node = theContext()->getNode();

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });
    ++expected_subphase;

    runInEpochCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });
    ++expected_subphase;

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_subphase_collective_with_non_2) {
  auto range = vt::Index1D(num_elms);

  auto this_node = theContext()->getNode();

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
      }
    });
    ++expected_subphase;

    runInEpochCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
      }
    });

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });
    ++expected_subphase;

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_subphase_collective_nested_with_non) {
  auto range = vt::Index1D(num_elms);

  auto this_node = theContext()->getNode();

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
      }
    });
    ++expected_subphase;

    runInEpochCollective([&]{
      runSubphaseCollective([&]{
        if (this_node == 0) {
          c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
          o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
          c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
          o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
        }
      });
      ++expected_subphase;

      runSubphaseCollective([&]{
        if (this_node == 0) {
          c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
          o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
        }
      });
      ++expected_subphase;
    });

    runSubphaseCollective([&]{
      if (this_node == 0) {
        c_proxy.broadcast<MyMsg, colHandler>(expected_subphase);
        o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
      }
    });
    ++expected_subphase;

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_chainset_no_subphases) {
  auto range = vt::Index1D(num_elms);

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index1D>> chains
    = std::make_unique<vt::messaging::CollectionChainSet<vt::Index1D>>(c_proxy);

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runInEpochCollective([&]{
      chains->nextStepCollective("first", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });

      chains->nextStepCollective("second", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });

      chains->nextStepCollective("third", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });

      chains->phaseDone();
    });

    runInEpochCollective([&]{
      o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
    });

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_chainset_subphases_1) {
  auto range = vt::Index1D(num_elms);

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index1D>> chains
    = std::make_unique<vt::messaging::CollectionChainSet<vt::Index1D>>(c_proxy);

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runInEpochCollective([&]{
      chains->nextStepCollectiveSubphase("first", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("second", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("third", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->phaseDone();
    });

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_chainset_subphases_2) {
  auto range = vt::Index1D(num_elms);

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index1D>> chains
    = std::make_unique<vt::messaging::CollectionChainSet<vt::Index1D>>(c_proxy);

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runInEpochCollective([&]{
      o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
    });

    runInEpochCollective([&]{
      chains->nextStepCollectiveSubphase("first", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("second", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("third", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->phaseDone();
    });

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_chainset_subphases_3) {
  auto range = vt::Index1D(num_elms);

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index1D>> chains
    = std::make_unique<vt::messaging::CollectionChainSet<vt::Index1D>>(c_proxy);

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runInEpochCollective([&]{
      chains->nextStepCollectiveSubphase("first", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("second", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("third", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->phaseDone();
    });

    runInEpochCollective([&]{
      o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
    });

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_chainset_subphases_with_rooted) {
  auto range = vt::Index1D(num_elms);

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index1D>> chains
    = std::make_unique<vt::messaging::CollectionChainSet<vt::Index1D>>(c_proxy);

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runInEpochCollective([&]{
      chains->nextStepCollectiveSubphase("first", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollective("second", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });

      chains->nextStepCollectiveSubphase("third", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->phaseDone();
    });

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

TEST_F(TestSubphaseManagement, test_collective_and_chainset_subphases) {
  auto range = vt::Index1D(num_elms);

  auto o_proxy = vt::theObjGroup()->makeCollective<MyObjgrp>();

  auto c_proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index1D>> chains
    = std::make_unique<vt::messaging::CollectionChainSet<vt::Index1D>>(c_proxy);

  PhaseType n_subphases = 0;
  for (int phase = 0; phase < num_phases; phase++) {
    PhaseType expected_subphase = 0;

    runInEpochCollective([&]{
      chains->nextStepCollectiveSubphase("first", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("second", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->phaseDone();
    });

    runSubphaseCollective([&]{
      o_proxy.broadcast<MyMsg, &MyObjgrp::handler>(expected_subphase);
    });
    ++expected_subphase;

    runInEpochCollective([&]{
      chains->nextStepCollectiveSubphase("third", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->nextStepCollectiveSubphase("fourth", [=](vt::Index1D idx) {
        return c_proxy(idx).template send<MyMsg, colHandler>(expected_subphase);
      });
      ++expected_subphase;

      chains->phaseDone();
    });

    if (phase == 0) {
      n_subphases = expected_subphase + 1;
    } else {
      EXPECT_EQ(n_subphases, expected_subphase + 1);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto lbdh = theNodeLBData()->getLBData();
    ASSERT_TRUE(lbdh->node_data_.find(phase) != lbdh->node_data_.end());
    auto &phase_data = lbdh->node_data_.at(phase);
    for (auto &obj_data : phase_data) {
      EXPECT_EQ(obj_data.second.subphase_loads.size(), n_subphases);
    }
  }
}

}}}} // end namespace vt::tests::unit::subphase

#endif /*vt_check_enabled(lblite)*/
